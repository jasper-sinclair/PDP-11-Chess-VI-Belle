# PDP-11-Chess-VI-Belle
Groundbreaking 1970s/1980s era chess machine, now with full UCI support

## About Belle:
Developed by Ken Thompson and Joe Condon at Bell Labs in the late 70s/early 80s, the original Belle
was a pioneering, brute-force, special-purpose hardware chess machine.Key

## Features
It was the first machine to achieve master-level play (2250 USCF rating in 1983) and won the
ACM North American Computer Chess Championship five times.

## Port
This code has been ported and updated for modern systems by Jim Ablett, with recent fixes to its move
encoding, buffer overflows, and console mode. The port, which often runs via WinBoard, features an
integrated opening book (book.dat) and handles algebraic notation.

See: https://talkchess.com/viewtopic.php?t=86164

## Additions/Changes:

This repository contains Jim's orignal port and the following additions/changes:
- Full UCI support
- Parse FEN positions
- Call create_book() directly from UCI command line (previously this module was an independent program).
- Formatting -> 4 space tabs replaced with 2 space tabs
- Clang local variable and function parameter const warnings resolved

Nothing else has been altered, as every effort has been taken to change as little as possible in an effort to preserve the original programming of this historic engine, especially concerning the heart of the program (movegen, search, eval, etc.)

## Protocol modes
The engine supports 3 modes: console, winboard, and UCI.
Protocol and play options are selected via command-line parameters.

For example:

- chess6.exe -uci -nobook -post
- chess6.exe -wb -nobook -post

You can use included wb.bat or uci.bat to start the engine this way.

## Console Mode
Console mode is the default if starting the engine without parameters-> "chess6.exe", after that just hit ENTER to start a game.

In console mode, you simply make your move in algebraic format, for ex: e2e4 then hit enter.
After the engine announces it's move, hit ENTER again to see the board reprentation.


## Arena
Choose Engines then Manage from the drop down menu.

Add -uci -nobook -post (or whatever options you want) to the engine's Command Line Parameter input field.

![Arena Screenshot](arena.png)

Have fun-
Jasper
