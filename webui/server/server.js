const express = require('express')
const cors = require('cors')

const app = express()

app.use(cors())
app.use(express.json())

async function getBestMove(fen) {
    const { spawn } = require('child_process');

    return new Promise((resolve, reject) => {
        const engine = spawn('../../engine/cli', ['best', fen]);

        let output = '';

        engine.stdout.on('data', (data) => {
            output += data.toString();
        });

        engine.on('close', (code) => {
            if (code !== 0) {
                reject(new Error(`Exit code ${code}`));
            } else {
                resolve(output.trim());
            }
        });

        engine.on('error', reject);
    });
}

app.post('/api/bestmove', async (req, res) => {
  try {
    const move = await getBestMove(req.body.fen);
    console.log("Engine plays:", move);
    res.json({ move: move });
  } catch (err) {
    console.error(err);
    res.status(500).send("Engine error");
  }
})

app.post('/api/msg', (req, res) => {
  const msg = req.body.msg
  console.log(msg);
  res.json({result:"okay"})
})

app.listen(3000, () => {
  console.log('Engine server running on http://localhost:3000')
})