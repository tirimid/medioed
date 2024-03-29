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

* A shell environment (for program execution)
* mincbuild (for build, optional)
* Make (for build, optional)

## Management

* To build the program, run `mincbuild` or `make`
* To install the program, run `./install.sh`
* To uninstall the program from the system, run `./uninstall.sh`

## Usage

After installation: either run `medioed` on its own to open the medioed greeter,
or `medioed file.txt other.txt ...` to open all specified files as buffers. From
there, edit the files as necessary.

## Contributing

To contribute, please open a pull request with your changes or an issue with a
suggestion. As a rule of thumb, I will be more likely to accept pull requests
which fix bugs or improve compatiblity rather than those which add new features.
If you want to add a feature, I would prefer you opened an issue - however I
will accept pull requests if I like them.
