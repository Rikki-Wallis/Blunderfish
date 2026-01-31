import engine

WHITE = 0
BLACK = 1



position = engine.Position.decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
position.display(True)

for bb in position.sides[WHITE].bb:
    print(bin(bb))
print(position.sides[WHITE].bb)

#for w_piece in position.sides[WHITE]:
    

