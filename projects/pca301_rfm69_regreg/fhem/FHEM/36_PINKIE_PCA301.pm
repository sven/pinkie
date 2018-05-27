#
# 36_PINKIE_PCA301.pm
#
# Licensed under the FHEM license or if possible under the MIT license, see
# PINKIE license file for details.
#
# Currently there is no documentation available.
#
package main;

use strict;
use warnings;


#
# Constants
#
use constant {
    # PCA301 constants
    PINKIE_PCA301_CMD_POLL => 1,
    PINKIE_PCA301_CMD_ON => 2,
    PINKIE_PCA301_CMD_OFF => 3,
    PINKIE_PCA301_CMD_IDENT => 4,
    PINKIE_PCA301_CMD_TIMEOUT_RX => 5,
    PINKIE_PCA301_CMD_PAIR => 6,
    PINKIE_PCA301_CMD_SEND_BUDGET => 7,
    PINKIE_PCA301_CMD_TIMEOUT_TX => 8,
    PINKIE_PCA301_CMD_STATS_RESET => 9,
};


#
# Prototypes
#
sub PINKIE_PCA301_inst_set($);
sub PINKIE_PCA301_inst_get($);
sub PINKIE_PCA301_inst_detect($$);
sub PINKIE_PCA301_cmd_rx($$);


#
# PINKIE Device Descriptions
#
my %PINKIE_PCA301_Devices = (
    "pca301_common" => {

        "meta" => {
            "reg_base" => 4110,
        },

        "regs" => {

            "stat_rx" => {
                "reg" => 0,
                "datatype" => "uint16",
                "attr" => "ro",
                "unit" => "frames",
            },

            "stat_rx_crc_inval" => {
                "reg" => 2,
                "datatype" => "uint16",
                "attr" => "ro",
                "unit" => "frames",
            },

            "stat_rx_tout" => {
                "reg" => 4,
                "datatype" => "uint16",
                "attr" => "ro",
                "unit" => "requests",
            },

            "stat_tx" => {
                "reg" => 6,
                "datatype" => "uint16",
                "attr" => "ro",
                "unit" => "frames",
            },

            "stat_tx_err" => {
                "reg" => 8,
                "datatype" => "uint16",
                "attr" => "ro",
                "unit" => "frames",
            },

            "stat_tx_tout" => {
                "reg" => 10,
                "datatype" => "uint16",
                "attr" => "ro",
                "unit" => "frames",
            },

            "pair" => {
                "reg" => 12,
                "datatype" => "uint8",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 0,
            },

            "default_chan" => {
                "reg" => 13,
                "datatype" => "uint8",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 1,
            },

            "response_timeout" => {
                "reg" => 14,
                "datatype" => "uint16",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 500,
            },

            "request_retries" => {
                "reg" => 16,
                "datatype" => "uint8",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 2,
            },

            "auto_poll" => {
                "reg" => 17,
                "datatype" => "uint8",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 1,
            },

            "dump_frames" => {
                "reg" => 18,
                "datatype" => "uint8",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 0,
            },
        },
    },

    "pca301" => {

        "meta" => {
            "reg_base" => 4100,
            "inst_set" => \&PINKIE_PCA301_inst_set,
            "inst_get" => \&PINKIE_PCA301_inst_get,
        },

        "regs" => {
            "addr" => {
                "reg" => 0,
                "datatype" => "uint24",
                "rx_cb" => \&PINKIE_PCA301_inst_detect,
                "hide" => 1,
                "dev_init" => 0,                # don't init val on device
            },

            "chan" => {
                "reg" => 3,
                "datatype" => "uint8",
                "fhem_type" => PINKIE_FHEM_TYPE_ATTRIBUTE,
                "default" => 1,
                "dev_init" => 0,                # don't init val on device
            },

            "cons" => {
                "reg" => 4,
                "datatype" => "uint16",
            },

            "cons_tot" => {
                "reg" => 6,
                "datatype" => "uint16",
            },

            "rssi" => {
                "reg" => 8,
                "datatype" => "int8",
                "unit" => "dBm",
                "attr" => "ro",
            },

            "cmd" => {
                "reg" => 9,
                "datatype" => "uint8",
                "hide" => 1,
                "rx_cb" => \&PINKIE_PCA301_cmd_rx,
            },
        },

        "commands" => {

            "on" => {
                "reg" => {
                    "name" => "cmd",
                    "val" => PINKIE_PCA301_CMD_ON,
                },
            },

            "off" => {
                "reg" => {
                    "name" => "cmd",
                    "val" => PINKIE_PCA301_CMD_OFF,
                },
            },

            "identify" => {
                "reg" => {
                    "name" => "cmd",
                    "val" => PINKIE_PCA301_CMD_IDENT,
                },
            },

            "poll" => {
                "reg" => {
                    "name" => "cmd",
                    "val" => PINKIE_PCA301_CMD_POLL,
                },
            },

            "reset_stats" => {
                "reg" => {
                    "name" => "cmd",
                    "val" => PINKIE_PCA301_CMD_STATS_RESET,
                },
            },

            "toggle" => {},
        },

        "readings" => {

            "state" => {
                "default" => "unknown",
                "map" => {
                    "0" => "on",
                    "1" => "off",
                    "2" => "timeout",
                    "3" => "unknown",
                },
            },

        },

        "attributes" => {

            "devStateIcon" => 'on:on:toggle off:off:toggle unknown:unknown:off',

            "webCmd" => "on:off:toggle:state",
        },
    },
);


#
# Select PCA301 address and channel by instance
#
sub PINKIE_PCA301_inst_set($)
{
    my ($hash) = @_;
    my $name = $hash->{NAME};
    my $rr = $PINKIE_PCA301_Devices{$hash->{PINKIE_type}};
    my $reg = $rr->{addr}{reg};
    my $opts = $hash->{PINKIE_opts};
    my $pca301_addr = $opts->[4];

    PINKIE_LogInfo($name, "PINKIE_PCA301_inst_set: $pca301_addr");

    # set current instance
    $modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst} = $hash;

    # send address
    foreach ($pca301_addr =~ m/../g) {
        PINKIE_Send($hash, { "type" => "write", "reg" => $reg, "val" => hex($_) });
        $reg++;
    }

    # send channel
    PINKIE_Send($hash, { "type" => "write", "reg" => $reg, "val" => hex(1) });
}


#
# Return current PCA301 instance
#
sub PINKIE_PCA301_inst_get($)
{
    my ($hash) = @_;

    # find instance
    if (exists $modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst}) {
        return $modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst};
    }

    return undef;
}


#
# Find PCA301 instance and store it for later use
#
sub PINKIE_PCA301_inst_detect($$)
{
    my ($hash, $val) = @_;
    my $name = $hash->{NAME};
    my $id;
    my $inst;
    my $opts;

    # build option identifier
    $id = sprintf("%06x", $val);

    # iterate through PCA301 instances
    foreach my $dev_name (@{$modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_devs}}) {
        my $dev_hash = $modules{PINKIE}{PINKIE_devs}{$dev_name}{hash};
        my $dev_name = $dev_hash->{NAME};
        my $opts = $dev_hash->{PINKIE_opts};

        # check if PCA301 id matches
        if ($val == hex($opts->[4])) {
            PINKIE_LogInfo($name, "found PCA301 instance: \"$dev_name\", id: \"$id\"");

            # assign instance device
            $modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst} = $modules{PINKIE}{PINKIE_devs}{$dev_name}{hash};
            return;
        }
    }

    PINKIE_LogInfo($name, "no PCA301 instance found for id \"$id\"");
    $modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst} = undef;
}


#
# Parse received PCA301 command data
#
sub PINKIE_PCA301_cmd_rx($$)
{
    my ($hash, $val) = @_;
    my $name = $hash->{NAME};
    my $dev_hash;

    # get PCA301 instance
    if (!defined($modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst})) {
        PINKIE_LogErr($name, "command data received but no instance set");
        return;
    }
    $dev_hash = $modules{PINKIE}{PINKIE_insts}{pca301}{PINKIE_inst};

    PINKIE_LogDbg($name, "PINKIE_PCA301_cmd_rx: value: \"$val\"");

    # parse command
    if ($val == PINKIE_PCA301_CMD_ON) {
        PINKIE_LogInfo($name, "state = on");
        readingsSingleUpdate($dev_hash, "state", "on", 1);
        return;
    }

    if ($val == PINKIE_PCA301_CMD_OFF) {
        PINKIE_LogInfo($name, "state = off");
        readingsSingleUpdate($dev_hash, "state", "off", 1);
        return;
    }

    if ($val == PINKIE_PCA301_CMD_TIMEOUT_RX) {
        PINKIE_LogInfo($name, "state = RX timeout");
        readingsSingleUpdate($dev_hash, "state", "receive timeout", 1);
        return;
    }

    if ($val == PINKIE_PCA301_CMD_TIMEOUT_TX) {
        PINKIE_LogInfo($name, "state = TX timeout");
        readingsSingleUpdate($dev_hash, "state", "transmit timeout", 1);
        return;
    }
    if ($val == PINKIE_PCA301_CMD_SEND_BUDGET) {
        PINKIE_LogInfo($name, "state = no budget");
        readingsSingleUpdate($dev_hash, "state", "no budget", 1);
        return;
    }

    if ($val == PINKIE_PCA301_CMD_PAIR) {
        PINKIE_LogInfo($name, "state = paired");
        # pairing doesn't update the state */
        return;
    }
}


#
# Register device support in PINKIE
#
PINKIE_LogInfo("PINKIE_PCA301", "adding PCA301 device data");
PINKIE_device_add(\%PINKIE_PCA301_Devices);


# finalize module initialization
1;


=pod
=item summary PINKIE PCA301 support
=begin html

<a name="PINKIE_PCA301"></a>
<h3>PINKIE PCA301</h3>
<ul>
  TODO: Placeholder.
</ul>

=end html
=cut
