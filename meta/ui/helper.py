from enums import WHITE, BLACK, PIECE_NONE

def idx_to_piece(position, index):
    for colour in [WHITE, BLACK]:
        piece = 0
        for bb in position.sides[colour].bb:
            if (1 << index) & bb:
                return (colour, piece)
            piece += 1 
    return (0, 0)