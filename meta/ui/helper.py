from enums import WHITE, BLACK, PIECE_PNG_TABLE

def idx_to_piece(position, index):
    for colour in [WHITE, BLACK]:
        piece = 0
        for bb in position.sides[colour].bb:
            if (1 << index) & bb:
                return (colour, piece)
            piece += 1 
    return (0, 0)

def render_pieces(position, squares):
    for index, square in enumerate(squares):
        colour, piece = idx_to_piece(position, index)
        square["piece"] = PIECE_PNG_TABLE[colour][piece]