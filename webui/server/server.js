const express = require('express')
const { exec } = require('child_process')
const cors = require('cors')

const app = express()

app.use(cors())
app.use(express.json())

app.post('/api/bestmove', (req, res) => {
  const fen = req.body.fen
  const depth = Math.min(12, Math.max(6, parseInt(req.body.depth) || 10))

  exec(`../../engine/cli best "${fen}" ${depth}`, (err, stdout, stderr) => {
    if (err) {
      console.error(err)
      return res.status(500).json({ error: 'Engine failed' })
    }

    console.log("Engine plays " + stdout.trim());

    res.json({ move: stdout.trim() })
  })
})

app.post('/api/msg', (req, res) => {
  const msg = req.body.msg
  console.log(msg);
  res.json({result:"okay"})
})

app.listen(3000, () => {
  console.log('Engine server running on http://localhost:3000')
})