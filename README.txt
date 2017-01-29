The WORLD is controlled by a thread that iterates state.

Each TELNET connection is controlled by a thread that negotiates terminal
stuff. It relays keypresses to the controller and echoes a rendered screen to
the socket.

To build a game, implement a world according to world.h, then link it to main.c
and execute.

$ gcc -c mudpie.c
$ gcc -c world.c
$ gcc mudpie.o world.o -o game
$ ./game 1337
