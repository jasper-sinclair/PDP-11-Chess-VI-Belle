 
 Chess VI (Belle) 11/1/73
 
 Written by Ken Thompson and was a precursor to Belle. It was a program that ran on a DEC minicomputer (PDP-11).
 Originally written in an intermediate version of Unix C. At the time, C hadn't been finalized or publicly announced.
 It competed in the New Jersey open in 1972 and in the Atlanta ACM tournament of 1973.
 
 The program was officially "Belle" but the USCF wouldn't allow a single name to be entered into a tournament, so he put a "T" 
 on it with no particular meaning. T. Belle does not stand for "Tinkerbelle" or "Thompson Belle" or "Telephone Belle" (he worked at 
 the phone company during that time.) Mr. Thompson definetly says that "T." has "no particular meaning".
 
 The program was tested in the Westfield chess club and it got a 1420 ranting.
 It was retired around 1975 in favor of the more famous hardware based "Belle" chess computer / program.
 
 
 Code ported to modern systems by Jim Ablett  03/04/2026
 
 
 
Chess Engine User Commands


Game Control Commands
---------------------
Command	           Description

save	           Saves the current game state to a file named "chess.out"
restore	           Restores a previously saved game from "chess.out"
exit	           Exits the chess program
resign	           Resigns the current game (calls handle_hup)


Move Input Commands
-------------------
Command	           Description

[empty line]	   Pressing Enter with no input displays the current board position

Algebraic notation (e.g., e2e4)	Enters a move in algebraic format (enabled with mfmt on)
Descriptive notation (e.g., P/K4)	Enters a move in descriptive format (default when mfmt off)


Display Commands
----------------
Command	           Description

mfmt	           Toggles algebraic notation on/off
mfmt on	           Enables algebraic notation (e.g., "e2e4")
mfmt off	       Disables algebraic notation (uses descriptive format)
score	           Displays the current evaluation score for both sides
clock	           Shows the accumulated thinking time for both players
hint	           Calculates and displays the best move and its evaluation
repeat	           Replays all moves made so far in the game


Game Analysis Commands
----------------------
Command	           Description

test	           Toggles test mode (shows heuristic evaluations during search)
setup	           Enters board setup mode for custom positions
remove	           Takes back the last two moves (useful for analyzing alternatives)
first	           Sets the engine to play first (White) - only valid at start of game
manual	           Toggles manual mode (human plays both sides)


Time Control Commands
---------------------
Command	           Description

time	           Toggles time control on/off or shows current setting
time <seconds>     Sets the engine to think for specified seconds per move (e.g., time 10)


Opening Book Commands
---------------------
Command            Description

book	           Toggles opening book on/off
book on            Enables opening book
book off	       Disables opening book


Special Commands
----------------
Command	           Description

o-o or oo	Castles kingside in descriptive notation
o-o-o or ooo	Castles queenside in descriptive notation
alg	Alias for mfmt on (only valid at game start)


Xboard Commands
----------------
Command	           Description

-xboard	           Use xboard protocol
-nobook	           Disable opening book use


Command Examples
----------------

text
Chess
mfmt on              # Enable algebraic notation
time 10              # Set 10 seconds thinking time
book off             # Disable opening book
e2e4                 # Play e4 as White

# After Black plays, get a hint
hint                 # Shows best move and evaluation

# Check thinking time
clock                # Shows time used by each player

# Take back a move to try something else
remove               # Takes back last two moves

# Save the game for later
save                 # Saves to chess.out

# Exit the program
exit


Default Settings
    • Algebraic notation: OFF (uses descriptive format)
    • Opening book: ON
    • Time control: OFF (uses fixed depth search)
    • Manual mode: OFF (engine plays)
    • Test mode: OFF
    • Default depth: 8 ply
Notes
    • Commands are case-sensitive and must be entered in lowercase
    • Pressing Enter with no input shows the current board position
    • The first command only works at the start of a game
    • The remove command removes two moves (one full turn) to return to a previous position
    • When time control is enabled, the engine ignores the fixed depth setting and searches until time runs out
