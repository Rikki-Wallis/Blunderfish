from flask import Flask, render_template, jsonify

import engine
from enums import WHITE, BLACK, PIECE_PNG_TABLE
import helper

app = Flask(__name__)

def run_engine():
    # Run c++ engine
    pass

@app.route("/")
def index():
    # Create intial position
    position = engine.Position.decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    
    squares = []
    
    for rank in range(0, 8):
        for file in range(0, 8):
            is_light = (rank + file) % 2 == 0
            file_idx = rank*8 + file
            
            colour, piece = helper.idx_to_piece(position, file_idx)
            squares.append({
                "name": file_idx,
                "color": "light" if is_light else "dark",
                "piece": PIECE_PNG_TABLE[colour][piece]
            })
            
    return render_template("index.html", squares=squares)

@app.route("/data")
def data():
    output = run_engine()
    return jsonify({"output": output})

if __name__ == "__main__":
    app.run(debug=True)
