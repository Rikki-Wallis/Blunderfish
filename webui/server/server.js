const express = require('express')
const cors = require('cors')

const app = express()

app.use(cors())
app.use(express.json())

const { spawn } = require('child_process');

function getBestMove(fen) {
    return new Promise((resolve, reject) => {
        const engine = spawn('../../engine/cli', ['best', fen]);

        let output = '';
        let errorOutput = '';

        engine.stdout.on('data', (data) => {
            output += data.toString();
        });

        engine.stderr.on('data', (data) => {
            errorOutput += data.toString();
        });

        engine.on('error', (err) => {
            reject(err); // spawn failure
        });

        engine.on('close', (code, signal) => {
            if (code === 0) {
                resolve(output.trim());
            } else {
                reject(new Error(
                    `Engine failed (code=${code}, signal=${signal})\n${errorOutput}`
                ));
            }
        });
    });
}

app.post('/api/bestmove', async (req, res) => {
    try {
        const fen = req.body.fen;
        const move = await getBestMove(fen);

        console.log("Engine plays", move);

        res.json({ move: move });
    } catch (err) {
        console.error("Engine error:", err);

        res.status(500).json({
            error: "Engine crashed",
            details: err.message
        });
    }
});

app.post('/api/msg', (req, res) => {
  const msg = req.body.msg
  console.log(msg);
  res.json({result:"okay"})
})

app.listen(3000, () => {
  console.log('Engine server running on http://localhost:3000')
})