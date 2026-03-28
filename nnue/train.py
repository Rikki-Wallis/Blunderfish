from model import *

import time
from torch.utils.data import DataLoader

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

total = os.path.getsize("aggregate.bin") // RECORD_SIZE
#total = 100000
split = int(total * 0.9)

train = NNUEDataset("aggregate.bin", 0, split)
val   = NNUEDataset("aggregate.bin", split, total)

print(f"Training on {len(train)} dataset samples")

def collate_fn(batch):
    white_idx, black_idx, targets = zip(*batch)

    def extract_features_and_indices(idxs):
        features = []
        indices = []

        for i, r in enumerate(idxs):
            features.extend(r)
            indices.extend([i]*len(r))

        return features, indices

    white_features, white_indices = extract_features_and_indices(white_idx)
    black_features, black_indices = extract_features_and_indices(black_idx)

    return (
        torch.tensor(white_features, dtype=torch.long),
        torch.tensor(white_indices, dtype=torch.long),
        torch.tensor(black_features, dtype=torch.long),
        torch.tensor(black_indices, dtype=torch.long),
        torch.stack(targets)
    )

train_loader = DataLoader(train, batch_size=1024, shuffle=True, num_workers=12,persistent_workers=True,collate_fn=collate_fn)
val_loader = DataLoader(val, batch_size=1024, num_workers=12,persistent_workers=True,collate_fn=collate_fn)

#device = "cpu"
model = NNUE()
#model = torch.compile(model)
model = model.to(device)
#model.load_state_dict(torch.load("model_epoch1_val0.491991.pt"))

num_epochs = 30

optimizer = torch.optim.AdamW(model.parameters(), lr=1e-3)
scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=num_epochs, eta_min=1e-6)
criterion = nn.BCEWithLogitsLoss() 

start = time.perf_counter()

for epoch in range(num_epochs):
    train_loss = 0

    for i, (white_features, white_indices, black_features, black_indices, target) in enumerate(train_loader):
        target = target.to(device)
        white_features = white_features.to(device)
        white_indices  = white_indices.to(device)
        black_features = black_features.to(device)
        black_indices  = black_indices.to(device)

        cur_batch_size = target.size(0)

        output = model(cur_batch_size, white_features, white_indices, black_features, black_indices)
        loss = criterion(output, target.float())

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        train_loss += loss.item()

        elapsed = time.perf_counter() - start
        if i % 20 == 0:
            print(f"Batch {i+1}: {elapsed:.2f}s (loss={loss.item()})")
        start = time.perf_counter()

    train_loss /= len(train_loader)

    model.eval()
    with torch.no_grad():
        val_loss = 0

        for wf, wi, bf, bi, tv in val_loader:
            tv = tv.to(device)
            wf = wf.to(device)
            wi = wi.to(device)
            bf = bf.to(device)
            bi = bi.to(device)

            cur_batch_size = tv.size(0)

            val_loss += criterion(model(cur_batch_size, wf, wi, bf, bi), tv.float()).item()

        val_loss /= len(val_loader)

    print(f"Epoch {epoch+1} train: {train_loss:.6f} val: {val_loss:.6f} lr: {optimizer.param_groups[0]['lr']}")
    model.train()

    torch.save(model.state_dict(), f"model_epoch{epoch+1}_val{val_loss:.6f}.pt")

    scheduler.step()