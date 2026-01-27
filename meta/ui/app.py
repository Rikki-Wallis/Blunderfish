from flask import Flask, render_template, jsonify
import subprocess

app = Flask(__name__)

def run_engine():
    # Run c++ engine
    pass

@app.route("/")
def index():
    
    squares = []
    
    for rank in range(0, 8):
        for file in range(0, 8):
            is_light = (rank + file) % 2 == 0
            file_idx = rank + file
            squares.append({
                "name": file_idx,
                "color": "light" if is_light else "dark"
            })
    
    return render_template("index.html", squares=squares)

@app.route("/data")
def data():
    output = run_engine()
    return jsonify({"output": output})

if __name__ == "__main__":
    app.run(debug=True)
