# Non-Deterministic Finite Automata Project

## Epsilon-NFA to NFA Converter

Takes an arbitrary Epsilon-NFA, formatted as an input file, fmt specified below,
and converts it to a regular NFA by removing all epsilon transitions from the
NFA's delta function.

## NFA Simulator

Takes an arbitrary Epsilon-NFA, formatted as an input file, fmt specified below,
via command line argument, and reads standard input for NFA input strings.
When an input string is submitted with \[RETURN/ENTER\], the NFA will be simulated
on the given input, and will output either, "accept", or, 
"reject", to the standard output, depending on whether or not the NFA accepts or
rejects on the given input, respectively.
