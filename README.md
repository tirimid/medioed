# medioed

## Introduction

medioed, or (Medio)cre Text (Ed)itor, is a text editor program I'm making
because I want to. The user experience should be somewhat similar to a less
configurable (and substantially more minimal) GNU Emacs. Basically, I'm trying
to make *my* perfect text editor, with all the features that *I* want from one.
Thus, this probably won't be very useful to you, unless you and I happen to be
the same person.

## Dependencies

Software / system dependencies are:

* A shell environment for program execution

## Management

* To build the program, run `mincbuild`
* To install the program, run `./install.sh`
* To uninstall the program from the system, run `./uninstall.sh`

## Usage

After installation: either run `medioed` on its own to open the medioed greeter,
or `medioed file.txt other.txt ...` to open all specified files as buffers. From
there, edit the files as necessary.

## Contributing

I am not accepting pull requests from anyone other than myself. Feel free to
fork this project and make your own version.
