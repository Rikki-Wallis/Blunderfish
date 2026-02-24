import {Chess} from "chess.js"

const game = new Chess();
let board;

function formatMovesList() {
  const movesEl = document.getElementById('moves');
  if (!movesEl) return;
  movesEl.innerHTML = '';
  const history = game.history({verbose: false});
  for (let i = 0; i < history.length; i += 2) {
    const moveNum = Math.floor(i/2) + 1;
    const white = history[i] || '';
    const black = history[i+1] || '';
    const li = document.createElement('li');
    li.textContent = `${moveNum}. ${white} ${black}`.trim();
    movesEl.appendChild(li);
  }
}

function updateStatus() {
  const status = document.getElementById('status');
  if (!status) return;
  if (game.isCheckmate()) status.textContent = 'Checkmate';
  else if (game.isStalemate()) status.textContent = 'Stalemate';
  else status.textContent = (game.turn() === 'w') ? 'White to move' : 'Black to move';
}

function updateFen() {
  const fenEl = document.getElementById('fen');
  if (!fenEl) return;
  fenEl.value = game.fen();
}

function loadFen() {
  const fenEl = document.getElementById('fen');
  if (!fenEl) return;
  
  const fenString = fenEl.value.trim();
  
  if (!fenString) {
    alert('Please enter a FEN string');
    return;
  }

  try {
    game.load(fenString);
    
    // Verify the FEN was actually loaded by checking if it matches
    const loadedFen = game.fen();
    if (loadedFen !== fenString) {
      alert('Invalid FEN string');
      return;
    }
  } catch (e) {
    console.error('Error loading FEN:', e.message);
    alert('Error: ' + e.message);
    return;
  }

  board.setPosition(game.fen());
  formatMovesList();
  updateStatus();
  updateFen();
  continueGame();
}

function check_game_over() {
  if (game.isCheckmate()) {
    alert("Game over by checkmate.");
  }
  else if (game.isStalemate()) {
    alert("Draw by stalemate.");
  }
  else if (game.isDrawByFiftyMoves()) {
    alert("Draw by 50 moves.");
  }
}

async function send_message(msg) {
  await fetch('/api/msg', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({msg: msg })
  });
}

async function makeMove() {
  try {
    const m = await fetch('/api/bestmove', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({fen: game.fen() })
    });

    if (!m.ok) {
      console.error('API error:', m.status, m.statusText);
      return;
    }

    const moveJson = await m.json();
    console.log('API response:', moveJson);
    const move = moveJson.move;

    if (!move) {
      console.error('No move in response');
      return;
    }

    let possibleMoves = game.moves();

    if (possibleMoves.length === 0) {
      console.log('No possible moves available');
      return;
    }

    if (!possibleMoves.includes(move)) {
      console.error('Engine gave illegal move ' + move);
      alert("Engine gave illegal move " + move);
      return;
    }

    console.log(possibleMoves);
    console.log("Engine plays " + move);

    game.move(move);
    
    board.setPosition(game.fen());
    formatMovesList();
    updateStatus();
    updateFen();

    window.setTimeout(check_game_over, 500);
    window.setTimeout(continueGame, 600);
  } catch (err) {
    console.error('Error in makeMove:', err);
  }
}

// Continue the game: if it's Black's turn, query the API; otherwise wait for player
async function continueGame() {
  if (game.isGameOver()) return;
  if (game.turn() === 'w') {
    // White's turn - wait for player
    return;
  }
  // Black's turn - query API and play
  await makeMove();
}

document.addEventListener('DOMContentLoaded', () => {
  board = document.querySelector('#board');

  // Load FEN button handler
  const loadFenBtn = document.getElementById('load-fen');
  if (loadFenBtn) {
    loadFenBtn.addEventListener('click', loadFen);
  }

  // Allow Enter key to load FEN
  const fenEl = document.getElementById('fen');
  if (fenEl) {
    fenEl.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        loadFen();
      }
    });
  }

  // Download PGN button handler
  const downloadBtn = document.getElementById('download-pgn');
  if (downloadBtn) {
    downloadBtn.addEventListener('click', () => {
      // build PGN with headers and set Black to "Blunderfish"
      const rawPgn = game.pgn() || '';
      // strip existing header block if present (headers are separated from moves by a blank line)
      let movesOnly = rawPgn;
      const sepIndex = rawPgn.indexOf('\n\n');
      if (sepIndex !== -1) {
        movesOnly = rawPgn.slice(sepIndex + 2).trim();
      }
      // remove any trailing result from moves (we'll add our own)
      movesOnly = movesOnly.replace(/\s*(1-0|0-1|1\/2-1\/2|\*)$/,'').trim();

      const d = new Date();
      const dateStr = `${d.getFullYear()}.${String(d.getMonth()+1).padStart(2,'0')}.${String(d.getDate()).padStart(2,'0')}`;

      let result = '*';
      if (game.isCheckmate()) {
        result = (game.turn() === 'w') ? '0-1' : '1-0';
      } else if (game.isStalemate() || game.isDrawByFiftyMoves()) {
        result = '1/2-1/2';
      }

      const headers = [
        '[Event "Casual Game"]',
        '[Site "?"]',
        `[Date "${dateStr}"]`,
        '[Round "?"]',
        '[White "White"]',
        '[Black "Blunderfish"]',
        `[Result "${result}"]`,
        ''
      ].join('\n');

      const pgn = headers + (movesOnly ? (movesOnly + ' ') : '') + result;
      const blob = new Blob([pgn], { type: 'text/plain;charset=utf-8' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'game.pgn';
      document.body.appendChild(a);
      a.click();
      a.remove();
      URL.revokeObjectURL(url);
    });
  }

  if (!board) {
    console.warn('chess-board element not found');
    return;
  }

  // simple control buttons
  const newGameBtn = document.getElementById('new-game');
  if (newGameBtn) newGameBtn.addEventListener('click', () => { game.reset(); board.setPosition(game.fen()); formatMovesList(); updateStatus(); });
  // undo button removed from UI

  board.addEventListener('drag-start', (e) => {
    const {source, piece, position, orientation} = e.detail;

    // do not pick up pieces if the game is over
    if (game.isGameOver()) {
      e.preventDefault();
      return;
    }

    // only pick up pieces for White
    if (piece.search(/^b/) !== -1) {
      e.preventDefault();
      return;
    }
  });

  board.addEventListener('drop', async (e) => {
  const {source, target, setAction} = e.detail;

  // see if the move is legal
  const move = game.move({
    from: source,
    to: target,
    promotion: 'q' // NOTE: always promote to a queen for example simplicity
  });

  // illegal move
  if (move === null) {
    setAction('snapback');
    return;
  }

  await send_message("Player plays " + move.san);

  formatMovesList();
  updateStatus();
  updateFen();
  check_game_over();

  // continue game: if it's black's turn, engine will play
  continueGame();
});

// update the board position after the piece snap
// for castling, en passant, pawn promotion
board.addEventListener('snap-end', (e) => {
  board.setPosition(game.fen());
});

  // initialize UI state
  board.setPosition(game.fen());
  formatMovesList();
  updateStatus();
  updateFen();
  // start game - if it's black's turn, engine plays first
  continueGame();

});