#
# 36_PINKIE.pm
#
# Copyright (c) 2017-2018, Sven Bachmann <dev@mcbachmann.de>
#
# Licensed under the FHEM license or if possible under the MIT license, see
# PINKIE license file for details.
#
# Currently there is no documentation available.
#

package main;

use strict;
use warnings;
use Time::HiRes qw(usleep);
use Data::Dumper;
use Data::Hexdumper qw(hexdump);


#
# Constants
#
use constant {
    PINKIE_REG_ADDR_MAX => 0xffff,

    # FHEM constants
    PINKIE_FHEM_TYPE_READING => 0,
    PINKIE_FHEM_TYPE_ATTRIBUTE => 1,
};


#
# Prototypes
#
sub PINKIE_LogErr($$);
sub PINKIE_LogWarn($$);
sub PINKIE_LogInfo($$);
sub PINKIE_LogTx($$);
sub PINKIE_LogRx($$);
sub PINKIE_LogDbg($$);
sub PINKIE_Initialize($);
sub PINKIE_Define($$);
sub PINKIE_Undef($$);
sub PINKIE_Get($$@);
sub PINKIE_Set($$@);
sub PINKIE_Read($);
sub PINKIE_Send($$);
sub PINKIE_Device($$);
sub PINKIE_Write($$);
sub PINKIE_Notify($$);
sub PINKIE_Value_Conv($$$);
sub PINKIE_device_init($);
sub PINKIE_device_add($);
sub PINKIE_req_add($$$;$);


#
# PINKIE Device Descriptions
#
my %PINKIE_Devices = (
    "generic" => {

        "meta" => {
            "reg_base" => 0,
        },

        "regs" => {
            "id" => {
                "reg" => 0,
                "datatype" => "uint32",
                "attr" => "ro",
            },

            "version" => {
                "reg" => 4,
                "datatype" => "uint8",
            },

            "endianness" => {
                "reg" => 5,
                "datatype" => "uint8",
                "attr" => "ro",
                "map" => {
                    "0" => "little",
                    "1" => "big",
                },
            },
        },
    },

    "common" => {

        "meta" => {
            "reg_base" => 1000,
        },

        "regs" => {
            "NVS_CRC" => {
                "reg" => 0,
                "datatype" => "uint16",
                "attr" => "ro",
            },

            "NVS_commit" => {
                "reg" => 0,
                "datatype" => "uint8",
                "attr" => "wo",
                "map" => {
                    "0" => "none",
                    "1" => "write",
                },
            },

            "NVS_invalidate" => {
                "reg" => 1,
                "datatype" => "uint8",
                "attr" => "wo",
                "map" => {
                    "0" => "keep",
                    "1" => "invalidate",
                },
            },

            "NVS_RFM69_high_power_mode" => {
                "reg" => 2,
                "datatype" => "bool8",
                "map" => {
                    "0" => "disable",
                    "1" => "enable",
                }
            },

            "NVS_RFM69_temp_ofs" => {
                "reg" => 3,
                "datatype" => "int8",
            },
        },
    },

    "atmega" => {
        "meta" => {
            "reg_base" => 2000,
        },

        "regs" => {
            "temp" => {
                "reg" => 0,
                "datatype" => "uint16",
                "unit" => "°C",
                "attr" => "ro",
            },

            "voltage" => {
                "reg" => 2,
                "datatype" => "uint16",
                "unit" => "mV",
                "attr" => "ro",
            },

            "timestamp" => {
                "reg" => 4,
                "datatype" => "uint64",
                "unit" => "ms",
                "attr" => "ro",
            },
        },
    },

    "rfm69" => {
        "meta" => {
            "reg_base" => 3000,
        },

        "regs" => {
            "RFM69_temp" => {
                "reg" => 114,
                "datatype" => "uint8",
                "unit" => "°C",
                "attr" => "ro",
            },

            "RFM69_rssi" => {
                "reg" => 115,
                "datatype" => "int8",
                "unit" => "dBm",
                "attr" => "ro",
            },

            "RFM69_budget" => {
                "reg" => 117,
                "datatype" => "uint8",
                "unit" => "s",
                "attr" => "ro",
            },
        },
    },
);


#
# Datatype Width List
#
my %PINKIE_Datatypes = (
    "bool8" => {
        "width" => "1",
        "unpack" => "C",
    },
    "int8" => {
        "width" => "1",
        "unpack" => "c",
    },
    "int16" => {
        "width" => "2",
        "unpack" => "s",
    },
    "uint8" => {
        "width" => "1",
        "unpack" => "C",
    },
    "uint16" => {
        "width" => "2",
        "unpack" => "n",
    },
    "uint24" => {
        "width" => "3",
        "unpack" => "N",
    },
    "uint32" => {
        "width" => "4",
        "unpack" => "N",
    },
    "uint48" => {
        "width" => "6",
        "unpack" => "Q",
    },
    "uint64" => {
        "width" => "8",
        "unpack" => "Q>",
    },
);


#
# Global variables
#
my $PINKIE_flg_init = 0;                        # init done flag
my @PINKIE_Send_Fifo;                           # send fifo
my $PINKIE_Send_Msg = "";                       # message in send process


#
# Error logging
#
sub PINKIE_LogErr($$)
{
    my ($who, $msg) = @_;
    my ($package, $filename, $line) = caller;
    Log3 $who, 1, "[Errx] PINKIE ($who:$line) - $msg";
}


#
# Warning logging
#
sub PINKIE_LogWarn($$)
{
    my ($who, $msg) = @_;
    my ($package, $filename, $line) = caller;
    Log3 $who, 2, "[Warn] PINKIE ($who:$line) - $msg";
}


#
# Info logging
#
sub PINKIE_LogInfo($$)
{
    my ($who, $msg) = @_;
    my ($package, $filename, $line) = caller;
    Log3 $who, 2, "[Info] PINKIE ($who:$line) - $msg";
}


#
# TX logging
#
sub PINKIE_LogTx($$)
{
    my ($who, $msg) = @_;
    my ($package, $filename, $line) = caller;
    Log3 $who, 3, "[TxTx] PINKIE ($who:$line) - $msg";
}


#
# RX logging
#
sub PINKIE_LogRx($$)
{
    my ($who, $msg) = @_;
    my ($package, $filename, $line) = caller;
    Log3 $who, 4, "[RxRx] PINKIE ($who:$line) - $msg";
}


#
# Debug logging
#
sub PINKIE_LogDbg($$)
{
    my ($who, $msg) = @_;
    my ($package, $filename, $line) = caller;
    Log3 $who, 5, "[Dbgx] PINKIE ($who:$line) - $msg";
}


#
# Return a device hash for a given device name
#
sub PINKIE_dev_hash_get($)
{
    my ($name) = @_;

    return $defs{$name};
}


#
# Initialize the module
#
sub PINKIE_Initialize($)
{
    my ($hash) = @_;

    require "$attr{global}{modpath}/FHEM/DevIo.pm";

    $hash->{DefFn} = "PINKIE_Define";
    $hash->{UndefFn} = "PINKIE_Undef";
    $hash->{GetFn} = "PINKIE_Get";
    $hash->{SetFn} = "PINKIE_Set";
    $hash->{ReadFn} = "PINKIE_Read";
    $hash->{WriteFn} = "PINKIE_Write";
    $hash->{NotifyFn} = "PINKIE_Notify";
    $hash->{NOTIFYDEV} = "TYPE=(PINKIE)";
    $hash->{AttrList} = $readingFnAttributes;

    Log3 "PINKIE", 3, "PINKIE - initialized";
}


#
# Define handler
#
sub PINKIE_Define($$)
{
    my ($hash, $def) = @_;
    my @opts = split("[ \t][ \t]*", $def);
    my $cnt_opts = @opts;
    my $name = $opts[0];
    my $type;
    my $dev_reg;
    my $host;
    my $dev;

    PINKIE_LogDbg($name, "Define");

    # check for minimal parameter count to detect device type
    if ($cnt_opts < 3) {
        PINKIE_LogInfo($name, "wrong number of arguments for define");
        return "wrong number of arguments for define";
    }

    # assign type
    $type = $opts[2];
    if ($type ne "base") {
        $host = $type;
        $type = $opts[3];
    }

    if (($type ne "base") && (!defined($PINKIE_Devices{$type}))) {
        PINKIE_LogInfo($name, "unknown device type \"$type\"");
        return "unknown device type \"$type\"";
    }

    # store common data
    $modules{PINKIE}{PINKIE_devs}{$name}{hash} = $hash;
    $hash->{PINKIE_type} = $type;
    $hash->{PINKIE_opts} = \@opts;

    # initialize type specific data
    if ($type eq "base") {

        # assign device
        $hash->{DeviceName} = $opts[3];

        # make sure device is really closed
        DevIo_CloseDev($hash);

        # open device
        DevIo_OpenDev($hash, 0, \&PINKIE_Device);

        # create sub-devices hash
        $modules{PINKIE}{PINKIE_hosts}{$name}{PINKIE_devs} = [];

    } else {

        # store host name in device
        $hash->{PINKIE_host} = $host;

        # assign device to host
        push (@{$modules{PINKIE}{PINKIE_hosts}{$host}{PINKIE_devs}}, $name);

        # check if instance must be mapped directly to register
        if ($PINKIE_Devices{$type}{meta}{inst_get}) {
            push (@{$modules{PINKIE}{PINKIE_insts}{$type}{PINKIE_devs}}, $name);
        }

        # add type to register mapping
        $modules{PINKIE}{PINKIE_types}{$type} = $hash;

        # assign IO device
        AssignIoPort($hash, $host);

        # assign readings and attributes from registers
        foreach $dev_reg (keys %{$PINKIE_Devices{$type}{"regs"}}) {

            my $it_reg = $PINKIE_Devices{$type}{"regs"}{$dev_reg};

            if ((defined($it_reg->{"fhem_type"})) && (PINKIE_FHEM_TYPE_ATTRIBUTE == $it_reg->{"fhem_type"})) {

                # add attribute to device
                addToDevAttrList($name, $dev_reg);
            }
        }
    }

    PINKIE_LogInfo($name, "defined device with type \"$type\"");

    return undef;
}


#
# Undefine handler
#
sub PINKIE_Undef($$)
{
    my ($hash, $name) = @_;

    PINKIE_LogInfo($name, "Undef");

    # close device if PINKIE base was selected
    if ($hash->{PINKIE_type} eq "base") {
        DevIo_CloseDev($hash);
    }

    PINKIE_LogInfo($name, "undefine maybe not fully implemented");

    return undef;
}


#
# Get handler
#
sub PINKIE_Get($$@)
{
    my ($hash, $name, $opt, @args) = @_;
    my @opts = ();
    my $rr_list = $PINKIE_Devices{$hash->{PINKIE_type}};

    PINKIE_LogDbg($name, "Get");

    if (!defined($opt)) {
        return "\"get $name\" needs at least one argument";
    }

    PINKIE_LogDbg($name, "Get - 1");

    # check if FHEM queries arguments
    if ($opt eq "?") {

        # build list of get-able options
        PINKIE_LogDbg($name, "Get - 2");

        foreach my $reg_name (keys %{$rr_list->{regs}}) {
            my $rr = $rr_list->{regs}{$reg_name};

            PINKIE_LogDbg($name, "Get - 3");

            # skip entries that should be hidden
            if (defined($rr->{"hide"}) and ($rr->{"hide"} == 1)) {
                next;
            }

            # skip write-only entries
            if (defined($rr->{"attr"}) and ($rr->{"attr"} eq "wo")) {
                next;
            }

            unshift(@opts, "$reg_name:noArg");
        }

        PINKIE_LogDbg($name, "Get - 4");

        return "Unknown argument $opt, choose one of " . join(" ", @opts);
    }

    # enqueue get request
    PINKIE_LogInfo($name, "PINKIE_req_add: get $opt");
    return PINKIE_req_add($hash, "get", $opt);
}


#
# Set handler
#
sub PINKIE_Set($$@)
{
    my ($hash, $name, $opt, @args) = @_;
    my @opts = ();
    my $rr_list = $PINKIE_Devices{$hash->{PINKIE_type}};
    my $val;
    my $reg_name = undef;

    PINKIE_LogDbg($name, "Set");

    if (!defined($opt)) {
        return "\"set $name\" needs at least one argument";
    }

    # check if FHEM queries arguments
    if ($opt eq "?") {

        # build list of set-able options
        foreach my $cmd (keys %{$rr_list->{commands}}) {
            my $cmd_data = $rr_list->{commands}{$cmd};
            my $cmd_arg = $cmd;

            # add mapping values if available
            if (exists($cmd_data->{"map"})) {
                $cmd_arg .= ":" . join(",", values %{$cmd_data->{"map"}});
            } else {
                $cmd_arg .= ":noArg";
            }

            unshift (@opts, $cmd_arg);
        }

        return "Unknown argument $opt, choose one of " . join(" ", @opts);
    }

    # assign value
    $val = $args[0];
    if (!defined($val)) {
        $val = 0;
    }

    # handle 'toggle' command
    if ($opt eq 'toggle') {
        $opt = (ReadingsVal($name, "state", "on") eq "off") ? "on" :"off";
    }

    # get register name
    if (exists($rr_list->{commands}{$opt})) {
        if (exists($rr_list->{commands}{$opt}{reg})) {

            $reg_name = $rr_list->{commands}{$opt}{reg}{name};

            if (exists($rr_list->{commands}{$opt}{reg}{val})) {
                $val = $rr_list->{commands}{$opt}{reg}{val};
            }
        }
    }

    # enqueue set request
    return PINKIE_req_add($hash, "set", $reg_name, $val);
}


#
# IO Device handler
#
sub PINKIE_Device($$)
{
    my ($hash, $err) = @_;
    my $name = $hash->{NAME};

    PINKIE_LogInfo($name, "Device");

    if ($err) {
        Log3 $name, 4, "PINKIE ($name) - unable to connect to device: $err";
    }

    PINKIE_LogInfo($name, "device detected");
}


#
# Parse handler
#
# Handles data sent from the device to FHEM.
#
sub PINKIE_Read($)
{
    my ($hash) = @_;
    my $name = $hash->{NAME};
    my $buf = DevIo_SimpleRead($hash);
    my $hash_found = undef;
    my $reg_beg;
    my $reg_end;
    my $reg_width;
    my $reg_rel;
    my $reg;
    my $reg_name;
    my $val;
    my $rr_list;
    my $msg;
    my $dev;
    my $reg_set;
    my $reg_set_ofs;
    my $reg_ofs;
    my $inst;
    my $reg_found;
    my $fhem_type;
    my $dev_name;                               # name of found device

    PINKIE_LogDbg($name, "Read");

    if (!defined($buf)) {
        return undef;
    }

    # add received data to receive buffer
    $hash->{io}->{buf_rx} .= $buf;

    # check if received data contains a newline
    while ($hash->{io}->{buf_rx} =~ m/^(.*?)\r?\n(.*)/s) {

        # remove line from buffer
        $msg = $1;
        $hash->{io}->{buf_rx} = $2;

        PINKIE_LogRx($name, "rx: '$1'");

        # device initialised
        if ($msg =~ /System: ready/) {
            PINKIE_device_init($hash);
            next;
        }

        # check if send timeout must be cleared
        if ($msg =~ m/^ok$/) {
            if (defined($PINKIE_Send_Msg)) {

                # stop timeout
                RemoveInternalTimer($hash);

                # clear message
                $PINKIE_Send_Msg = undef;

                # trigger next send
                PINKIE_Send_Queued($hash);
            }
        }

        # parse line
        if ($msg !~ m/(\d+?): (0x.*?) /) {
            next;
        }

        $reg = $1;
        $val = hex($2);

        # find register by iterating over register sets
        $reg_set_ofs = PINKIE_REG_ADDR_MAX;
        $reg_set = undef;
        foreach my $reg_set_it (keys %PINKIE_Devices) {
            my $reg_diff;
            my $reg_set_base;

            # get base of current register set
            $reg_set_base = $PINKIE_Devices{$reg_set_it}{meta}{reg_base};

            # skip if base is above register
            if ($reg < $reg_set_base) {
                next;
            }

            # calculate relative register index
            $reg_diff = $reg - $reg_set_base;

            # find nearest register set
            if ($reg_diff < $reg_set_ofs) {
                $reg_set_ofs = $reg_diff;
                $reg_set = $reg_set_it;
            }
        }

        # check if a register set was found
        if (!defined($reg_set)) {
            PINKIE_LogWarn($name, "no register set found for received data");
            next;
        }
        PINKIE_LogDbg($name, "found register set: $reg_set");

        # find register in register set
        $reg_found = undef;
        foreach my $reg_it (keys %{$PINKIE_Devices{$reg_set}{regs}}) {
            my $reg_it_beg = $PINKIE_Devices{$reg_set}{regs}{$reg_it}{reg};
            my $reg_it_width;
            my $reg_it_end;

            # skip if register begin is above register
            if ($reg_set_ofs < $reg_it_beg) {
                next;
            }

            # get register width
            $reg_it_width = $PINKIE_Datatypes{$PINKIE_Devices{$reg_set}{regs}{$reg_it}{datatype}}->{"width"};

            # calculate register end
            $reg_it_end = $reg_it_beg + $reg_it_width - 1;

            # exit if register is below or matching register end
            if ($reg_set_ofs <= $reg_it_end) {
                $reg_found = $reg_it;
                $reg_end = $reg_it_end;
                $reg_ofs = $reg_set_ofs - $reg_it_beg;
                last;
            }
        }

        # check if register was found
        if (!defined($reg_found)) {
            PINKIE_LogWarn($name, "Read: no register found for received data");
            next;
        }
        PINKIE_LogDbg($name, "Read: found register $reg_set:$reg_found:$reg_ofs");

        # store register data
        $modules{PINKIE}{PINKIE_hosts}{$name}{PINKIE_reg_tmp}{$reg_set}{$reg_found}[$reg_ofs] = $val;

        # if not last register fragment then leave
        if ($reg_set_ofs < $reg_end) {
            next;
        }
        PINKIE_LogDbg($name, "Read: read all register fragments");

        # convert register fragments to value
        my $val_line = "";
        my $data_reg = $modules{PINKIE}{PINKIE_hosts}{$name}{PINKIE_reg_tmp}{$reg_set}{$reg_found};
        @{$data_reg} = reverse(@{$data_reg});

        # build hex string from value array
        for my $cnt (0 .. $#{$data_reg}) {

            PINKIE_LogDbg($name, "Read: $reg_set:$reg_found:$cnt");

            if (!defined(@{$data_reg}[$cnt])) {
                PINKIE_LogWarn($name, "PINKIE_Read: incomplete register read \"$reg_set:$reg_found\", dropping values");
                undef $modules{PINKIE}{PINKIE_hosts}{$name}{PINKIE_reg_tmp}{$reg_set}{$reg_found};
                $val_line = undef;
                last;
            }

            $val_line .= sprintf("%02x", @{$data_reg}[$cnt]);
        }

        # check if register read failed
        if (!defined($val_line)) {
            next;
        }

        # convert value to decimal
        $val_line = PINKIE_Value_Conv($hash, $PINKIE_Devices{$reg_set}{regs}{$reg_found}, $val_line);

        PINKIE_LogDbg($name, "PINKIE_Read: $reg_set:$reg_found = \"" . $val_line . "\"");

        # notify register rx callback if available
        if ($PINKIE_Devices{$reg_set}{regs}{$reg_found}{rx_cb}) {
            $PINKIE_Devices{$reg_set}{regs}{$reg_found}{rx_cb}($hash, $val_line);
        }

        # don't update hidden values
        if ((defined($PINKIE_Devices{$reg_set}{regs}{$reg_found}{"hide"})) and ($PINKIE_Devices{$reg_set}{regs}{$reg_found}{"hide"} == 1)) {
            next;
        }

        # get instance from helper if available
        PINKIE_LogDbg($name, "Read: get instance for received data");
        $inst = undef;
        if ($PINKIE_Devices{$reg_set}{meta}{inst_get}) {
            $inst = $PINKIE_Devices{$reg_set}{meta}{inst_get}->($hash);
        }
        else {
            # find instance that directly maps to register set
            if (exists $modules{PINKIE}{PINKIE_types}{$reg_set}) {
                $inst = $modules{PINKIE}{PINKIE_types}{$reg_set};
            }
        }

        # check if instance was found
        if (!defined($inst)) {
            PINKIE_LogWarn($name, "Read: no instance found for received data");
            next;
        }
        PINKIE_LogDbg($name, "Read: found instance $inst->{NAME}");

        # get register type
        if ($PINKIE_Devices{$reg_set}{regs}{$reg_found}{"fhem_type"}) {
            $fhem_type = $PINKIE_Devices{$reg_set}{regs}{$reg_found}{"fhem_type"};
        } else {
            $fhem_type = PINKIE_FHEM_TYPE_READING;
        }

        # handle type reading
        if (PINKIE_FHEM_TYPE_READING == $fhem_type) {
            PINKIE_LogInfo($name, "Read: $inst->{NAME}:reading:$reg_found = $val_line");
            readingsSingleUpdate($inst, $reg_found, $val_line, 1);
            next;
        }

        # handle type attribute
        if (PINKIE_FHEM_TYPE_ATTRIBUTE == $fhem_type) {
            PINKIE_LogInfo($name, "Read: $inst->{NAME}:attribute:$reg_found = $val_line");

            # only update attribute if value differs
            if ($attr{$inst->{NAME}}{$reg_found} eq $val_line) {
                next;
            }

            # update attribute value without calling notify
            $attr{$inst->{NAME}}{$reg_found} = $val_line;
            addStructChange("attr", $inst->{NAME}, "$inst->{NAME} $reg_found $val_line");
            next;
        }
    }

    return undef;
}


#
# Enqueue data in send list or handover data to base instance
#
sub PINKIE_Send($$)
{
    my ($hash, $msg) = @_;
    my $name = $hash->{NAME};
    my $pinkie_type = $hash->{"PINKIE_type"};

    PINKIE_LogInfo($name, "PINKIE_Send: sender $hash->{NAME}");

    if ($pinkie_type eq "base") {

        # push request to end of list
        push(@PINKIE_Send_Fifo, $msg);

        PINKIE_LogInfo($name, "PINKIE_Send -> PINKIE_Send_Queued");

        # trigger sending
        PINKIE_Send_Queued($hash);

    } else {

        PINKIE_LogInfo($name, "PINKIE_Send -> IOWrite");

        # add device hash to message
        $msg->{hash} = $hash;

        IOWrite($hash, $msg);
    }
}


#
# Send enqueued data to IO device
#
sub PINKIE_Send_Queued($)
{
    my ($hash) = @_;
    my $name = $hash->{NAME};
    my $dev_name;
    my $dev_type;
    my $msg;
    my $msg_text;

    PINKIE_LogInfo($name, "SendQueued");

    # check if another message is still in transmit
    if (defined($PINKIE_Send_Msg)) {
        return;
    }

    # get oldest message from message list
    $msg = shift(@PINKIE_Send_Fifo);
    if (!defined($msg)) {
        return;
    }

    # store message
    $PINKIE_Send_Msg = $msg;

    # get send instance name
    $dev_name = $msg->{hash}{NAME};

    # add register base address
    if (!defined $modules{PINKIE}{PINKIE_devs}{$dev_name}) {
        PINKIE_LogInfo($name, "PINKIE_Send_Queued: instance \"$dev_name\" not found");
        return;
    }
    $dev_type = $modules{PINKIE}{PINKIE_devs}{$dev_name}{hash}{PINKIE_type};

    # assign register base
    $msg->{reg} += $PINKIE_Devices{$dev_type}{meta}{reg_base};
    PINKIE_LogInfo($name, "register base: $PINKIE_Devices{$dev_type}{meta}{reg_base}");

    # transmit message
    $msg_text = "reg ";
    if ($msg->{"type"} eq "read") {
        $msg_text .= "read $msg->{reg}";
    } else {
        $msg_text .= "write $msg->{reg} $msg->{val}";
    }

    PINKIE_LogTx($name, "PINKIE_Send_Queued: \"$msg_text\"");

    DevIo_SimpleWrite($hash, "$msg_text\n", 0);
    usleep(50 * 1000);

    # wait for answer
    InternalTimer(gettimeofday() + 1, "PINKIE_Send_Timeout", $hash);
}


#
# Send timeout timer
#
sub PINKIE_Send_Timeout($)
{
    my ($hash) = @_;
    my $name = $hash->{NAME};

    PINKIE_LogInfo($name, "send timeout, clearing send FIFO");

    # check if message is available
    if (!defined($PINKIE_Send_Msg)) {
        return;
    }

    # set reading to 'timeout'
    readingsSingleUpdate($PINKIE_Send_Msg->{"hash"}, "state", "timeout", 1);

    # release current message
    $PINKIE_Send_Msg = undef;

    # drop send queue
    @PINKIE_Send_Fifo = ();
}


#
# IO Write handler
#
sub PINKIE_Write($$)
{
    my ($hash, $msg) = @_;
    my $name = $hash->{NAME};

    PINKIE_LogInfo($name, "PINKIE_Write -> PINKIE_Send_Queued");

    # push request to end of list
    push(@PINKIE_Send_Fifo, $msg);

    # trigger sending
    PINKIE_Send_Queued($hash);

    return undef;
}


#
# Register value converter
#
# Value parameter is given as decimal number.
#
sub PINKIE_Value_Conv($$$)
{
    my ($hash, $val_meta, $val) = @_;
    my $name = $hash->{NAME};
    my $conv;

    # pad 24 and 48 bit values for "unpack" to work correctly
    if (("uint24" eq $val_meta->{"datatype"}) or
        ("uint48" eq $val_meta->{"datatype"})) {
        $val = "00" . $val;
    }

    # pack hex values
    PINKIE_LogDbg($name, "PINKIE_Value_Conv: raw value: \"$val\", datatype: \"$val_meta->{datatype}\", unpack: \"$PINKIE_Datatypes{$val_meta->{datatype}}->{unpack}\"");
    $conv = pack('H*', $val);
    $conv = unpack($PINKIE_Datatypes{$val_meta->{"datatype"}}->{"unpack"}, $conv);
    PINKIE_LogDbg($name, "PINKIE_Value_Conv: converted value: $conv");

    # map value if mapping is available
    if (exists($val_meta->{map})) {
        $conv = $val_meta->{map}{$conv};
        PINKIE_LogDbg($name, "PINKIE_Value_Conv: mapped value = $conv");
    }

    return $conv;
}


#
# Initialize PINKIE device instances
#
sub PINKIE_device_init($)
{
    my ($hash_host) = @_;
    my $name_host = $hash_host->{NAME};

    if (0 != $PINKIE_flg_init) {
        return;
    }

    PINKIE_LogInfo($name_host, "PINKIE_device_init -> initialising PINKIE devices");

    # iterate through all devices
    foreach my $dev (keys %{$modules{PINKIE}{PINKIE_devs}}) {

        my $hash = $modules{PINKIE}{PINKIE_devs}{$dev}{hash};
        my $name = $hash->{NAME};
        my $type = $hash->{PINKIE_type};

        PINKIE_LogInfo($name_host, "init: $name");

        # assign readings and attributes from registers
        foreach my $dev_reg (keys %{$PINKIE_Devices{$type}{"regs"}}) {

            my $it_reg = $PINKIE_Devices{$type}{"regs"}{$dev_reg};

            if (!defined($it_reg->{"default"})) {
                next;
            }

            if ((defined($it_reg->{"fhem_type"})) && (PINKIE_FHEM_TYPE_ATTRIBUTE == $it_reg->{"fhem_type"})) {

                # add attribute to device
                addToDevAttrList($name, $dev_reg);

                # only initialize attribute if FHEM already knows it
                if (!defined($attr{$name}{$dev_reg})) {
                    fhem("attr $name $dev_reg $it_reg->{default}");
                }

                # send attribute value to device
                if ((!defined($it_reg->{"dev_init"})) || ($it_reg->{"dev_init"} != 0)) {
                    PINKIE_req_add($hash, "set", $dev_reg, $attr{$name}{$dev_reg});
                }

            } else {

                # type: PINKIE_FHEM_TYPE_READING

                if (defined($hash->{READINGS}{$dev_reg})) {
                    next;
                }

                readingsSingleUpdate($hash, $dev_reg, $it_reg->{"default"}, 1);
            }
        }
    }

    # clear message lock
    $PINKIE_Send_Msg = undef;

    # mark init as done
    $PINKIE_flg_init = 1;

    # trigger send
    PINKIE_Send_Queued($hash_host);
}


#
# Add device support to PINKIE
#
sub PINKIE_device_add($)
{
    my ($dev) = @_;

    @PINKIE_Devices{keys %{$dev}} = values %{$dev};
    PINKIE_LogInfo("PINKIE", "added device data");
}


#
# Notify Handler
#
sub PINKIE_Notify($$)
{
    my ($hash_own, $hash_dev) = @_;             # own and device hash
    my $name_own = $hash_own->{NAME};           # own name
    my $name_event;                             # event name (eg. global)
    my $events;                                 # event list

    # ignore callback if module is disabled
    if (IsDisabled($name_own)) {
        return "";
    }

    # assign name and events
    $name_event = $hash_dev->{NAME};
    $events = deviceEvents($hash_dev, 1);

    # skip if no events available
    if (!$events) {
        return;
    }

    foreach my $event (@{$events}) {
        if (!defined($event)) {
            continue;
        }

        # only process own events
        if ($event =~ /^ATTR (.*?) (.*?) (.*)$/) {
            if ($1 eq $name_own) {
                PINKIE_req_add($hash_own, "set", $2, $3);
            }
        }
    }
}


#
# Enqueue Request
#
sub PINKIE_req_add($$$;$)
{
    my $hash = shift;                           # device hash
    my $type = shift;                           # get or set
    my $reg = shift;                            # register
    my $val;                                    # set value
    my $name = $hash->{NAME};                   # device name
    my $rr_list = $PINKIE_Devices{$hash->{PINKIE_type}}; # register list
    my $rr;                                     # register entry

    # get value if request is set
    if ($type eq "set") {
        $val = shift;
    }

    # search element in register list
    if (!defined($rr_list->{regs}{$reg})) {
        PINKIE_LogDbg($name, "register $reg not found");
        return "Unknown register \"$reg\"";
    }
    $rr = $rr_list->{regs}{$reg};

    # check datatype
    if (!exists($PINKIE_Datatypes{$rr->{datatype}})) {
        PINKIE_LogDbg($name, "register $reg has no datatype");
        return "Unknown datatype for register \"$reg\"";
    }

    # call instance function if defined
    if ($rr_list->{meta}{inst_set}) {
        PINKIE_LogDbg($name, "setting instance");
        $rr_list->{meta}{inst_set}->($hash);
    }

    # use mapping on pre-write
    if (($type eq "set") && ($rr->{map})) {
        PINKIE_LogDbg($name, "mapping value");

        while ((my $key, my $map) = each (%{$rr->{map}})) {
            if ($map eq $val) {
                PINKIE_LogDbg($name, "mapped value \"$val\" to \"$key\"");
                $val = $key;
                last;
            }
        }
    }

    # use single writes for each datatype
    for (my $cnt = 0; $cnt < $PINKIE_Datatypes{$rr->{datatype}}->{"width"}; $cnt++) {
        PINKIE_LogDbg($name, "sending request");

        if ($type eq "set") {
            PINKIE_Send($hash, { "type" => "write", "reg" => $rr->{reg}, "val" => $val });
        } else {
            PINKIE_Send($hash, { "type" => "read", "reg" => ($rr->{reg} + $cnt) });
        }
    }

    return undef;
}


#
# Include devices
#
require "36_PINKIE_PCA301.pm";


# finalize module initialization
1;


=pod
=item summary PINKIE device management
=begin html

<a name="PINKIE"></a>
<h3>PINKIE</h3>
<ul>
  TODO: Placeholder.
</ul>

=end html
=cut
