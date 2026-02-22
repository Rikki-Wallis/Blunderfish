import {Chess} from "chess.js"

const board = document.querySelector('chess-board');
const game = new Chess();

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

async function makeMove() {
  const m = await fetch('/api/bestmove', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({fen: game.fen() })
  });

  const moveJson = await m.json();
  const move = moveJson.move;

  let possibleMoves = game.moves();

  if (possibleMoves.length === 0) {
    return;
  }

  if (!possibleMoves.includes(move)) {
    alert("Engine gave illegal move " + move);
  }

  console.log(possibleMoves);
  console.log("Engine plays " + move);

  game.move(move);
  
  board.setPosition(game.fen());

  window.setTimeout(check_game_over, 500);
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

  check_game_over();

  // make random legal move for black
  makeMove();
});

// update the board position after the piece snap
// for castling, en passant, pawn promotion
board.addEventListener('snap-end', (e) => {
  board.setPosition(game.fen());
});