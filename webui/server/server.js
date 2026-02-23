const express = require('express')
const { spawn } = require('child_process');
const cors = require('cors')

const app = express()

app.use(cors())
app.use(express.json())

app.post('/api/bestmove', (req, res) => {
  const fen = req.body.fen

  const engine = spawn('../../engine/cli', ['best', fen]);

  let output = '';

  engine.stdout.on('data', (data) => {
    output += data.toString();
  });

  engine.stderr.on('data', (data) => {
    console.error(data.toString());
    return res.status(500).json({ error: 'Engine failed' })
  });

  engine.on('close', (code) => {
    console.log('Exit code: ', code);
  });

  console.log("Engine plays " + output.trim());

  res.json({ move: output.trim() })
})

app.post('/api/msg', (req, res) => {
  const msg = req.body.msg
  console.log(msg);
  res.json({result:"okay"})
})

app.listen(3000, () => {
  console.log('Engine server running on http://localhost:3000')
})