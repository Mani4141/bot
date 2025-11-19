Chess Movement Assignment – CMPM 123

This project implements legal movement for pawns, knights, and the king using:

FEN board setup

BitboardElement and BitMove for knights and king

Manual rule logic for pawn forward movement + diagonal captures

Turn-based movement (white then black)

Capture logic (removes taken piece from board)

Implemented Pieces

Pawn:

Forward 1

Forward 2 from starting rank

Diagonal capture

Knight:

Full L-shape movement using precomputed bitboard table

Can jump over pieces

King:

One square in any direction
