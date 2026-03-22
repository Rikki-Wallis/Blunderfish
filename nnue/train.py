from nn import *

dataset = NNUEDataset("dataset")
#dataset.samples = dataset.samples[:100_000]
print(f"Training on {len(dataset)} dataset samples")

train, val = random_split(dataset, [0.9, 0.1])
train_loader = DataLoader(train, batch_size=1024, shuffle=True)
val_loader = DataLoader(val, batch_size=1024)

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
#device = "cpu"
model = NNUE()
model = torch.compile(model)
model = model.to(device)
#model.load_state_dict(torch.load("model_epoch10_val0.013123.pt"))

optimizer = torch.optim.AdamW(model.parameters(), lr=1e-3)
loss_fn = nn.MSELoss()

start = time.perf_counter()

for epoch in range(10):
    train_loss = 0

    i = 0

    for x, target in train_loader:
        x = x.to(device)
        target = target.to(device)

        pred = model(x)
        loss = loss_fn(pred, target.float())
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        train_loss += loss.item()

        if (i + 1) % 100 == 0:
            elapsed = time.perf_counter() - start
            print(f"Batch {i+1}: {elapsed:.2f}s per 100 batches")
            start = time.perf_counter()

        i += 1

    train_loss /= len(train_loader)

    model.eval()
    with torch.no_grad():
        val_loss = 0
        for xv, tv in val_loader:
            xv = xv.to(device)
            tv = tv.to(device)
            val_loss += loss_fn(model(xv), tv.float())
        val_loss /= len(val_loader)
    print(f"Epoch {epoch+1} train: {train_loss:.6f} val: {val_loss:.6f}")
    model.train()

    torch.save(model.state_dict(), f"model_epoch{epoch+1}_val{val_loss:.6f}.pt")