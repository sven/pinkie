# PINKIE - Framework for Tiny Devices

The PINKIE framework provides basic architecture abstraction and a build system
at the same time. External components can be easily added as modules.


## MIT License

See LICENSE file for details.


## Supported Architectures

  * Microchip ATmega8 (ARCH=atmega8)
  * Microchip ATmega328 (ARCH=atmega328)
  * Linux (ARCH=linux)


## Projects

### cli - ACyCLIC Commandline Integration

Demonstration of ACyCLIC CLI integration into PINKIE. See
[ACyCLIC](https://github.com/sven/acyclic) to learn more about the tiny
commandline interface with autocompletition and history support.


### reg\_cli - Register Access via CLI

Shows how to access mapped registers through CLI commands.


## Build Instructions

The common way to build PINKIE projects is to change into the project directory
and run make with the wanted architecture variable set.

```
cd projects/cli
make ARCH=atmega328
```


## Run Instructions

After a project has been build it depends on the architecture how to run it.
The following commands must be executed in the project directory, for example
in projects/cli.

  * atmega8: make ARCH=atmega8 avrdude
  * atmega328: make ARCH=atmega328 avrdude
  * linux: execute ./build/linux/pinkie

On Linux the binary runs on standard input and output. Other architectures like
the ATmega328 are using the serial port to communicate so an extra tool like
minicom is needed to interact with the application.
