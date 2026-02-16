const express = require('express')
const { exec } = require('child_process')
const cors = require('cors')

const app = express()

app.use(cors())
app.use(express.json())

app.post('/api/bestmove', (req, res) => {
  const fen = req.body.fen

  exec(`../../engine/blunderfish best "${fen}"`, (err, stdout, stderr) => {
    if (err) {
      console.error(err)
      return res.status(500).json({ error: 'Engine failed' })
    }

    console.log("suggesting " + stdout.trim());

    res.json({ move: stdout.trim() })
  })
})

app.listen(3000, () => {
  console.log('Engine server running on http://localhost:3000')
})