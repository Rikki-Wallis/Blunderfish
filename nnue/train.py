from model import *

import time
import multiprocessing
import matplotlib.pyplot as plt

from torch.utils.data import DataLoader

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
#device = "cpu"

total = os.path.getsize("aggregate.bin") // RECORD_SIZE
#total = 200
split = int(total * 0.9)

train = NNUEDataset("aggregate.bin", 0, split)
val   = NNUEDataset("aggregate.bin", split, total)

import multiprocessing

torch.multiprocessing.set_sharing_strategy('file_system')

print(f"Training on {len(train)} dataset samples")

batch_size = 256

from torch.utils.cpp_extension import load
ext = load(name="data_loader_ext", sources=["data_loader.cpp"], extra_cflags=['-O3'], verbose=True)

CUR_BLEND = multiprocessing.Value('f', 0.5)

def collate_fn(batch):
    return ext.collate_batch(batch, CUR_BLEND.value)

train_loader = DataLoader(train, batch_size=batch_size, shuffle=False, num_workers=12,persistent_workers=True,collate_fn=collate_fn, pin_memory=True)
val_loader = DataLoader(val, batch_size=batch_size, num_workers=12,persistent_workers=True,collate_fn=collate_fn)

model = NNUE()
model = torch.compile(model)
model = model.to(device)
#model.load_state_dict(torch.load("new.pt"))

print(f"Number of parameters: {sum(p.numel() for p in model.parameters())/1000:0.2f}K")

num_epochs = 60

optimizer = torch.optim.AdamW(model.parameters(), lr=1e-4)

scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
    optimizer,
    T_max=num_epochs,        # total epochs
    eta_min=3e-5     # final LR
)

criterion = nn.MSELoss() 

model.train()

start_blend = 0.0
end_blend = 0.0

start = time.perf_counter()

def lerp(a, b, t):
    return (1-t)*a + t*b

epoch = 0

if False:
    checkpoint = torch.load("checkpoint2_val0.003116.pt")
    model.load_state_dict(checkpoint["model"])
    optimizer.load_state_dict(checkpoint["optimizer"])
    scheduler.load_state_dict(checkpoint["scheduler"])
    epoch = checkpoint["epoch"]

train_losses = []
val_losses = []

fixed_lr = 5e-5

if fixed_lr is not None:
    for p in optimizer.param_groups:
        p["lr"] = fixed_lr

while epoch < num_epochs:
    train_loss = 0

    CUR_BLEND.value = lerp(start_blend, end_blend, epoch/(num_epochs-1))

    for i, (features, target) in enumerate(train_loader):
        features = features.to(device)
        target = target.to(device)

        optimizer.zero_grad()

        with torch.amp.autocast("cuda", dtype=torch.bfloat16):
            output = model(features)
            loss = criterion(output, target.float())

        loss.backward()
        optimizer.step()

        train_loss += loss.item()

        train_losses.append(loss.item())

        if i % 20 == 0:
            elapsed = time.perf_counter() - start
            pps = batch_size * 20 / elapsed
            print(f"Batch {i+1}: {pps/1000:.2f}K pps (loss={loss.item()})")
            start = time.perf_counter()

    if fixed_lr is not None:
        scheduler.step()

    train_loss /= len(train_loader)

    model.eval()
    with torch.no_grad():
        val_loss = 0

        for fv, tv in val_loader:
            fv = fv.to(device)
            tv = tv.to(device)

            val_loss += criterion(model(fv), tv.float()).item()

        val_loss /= len(val_loader)

    print(f"Epoch {epoch+1} train: {train_loss:.6f} val: {val_loss:.6f} (wdl blend: {CUR_BLEND.value})")
    model.train()

    val_losses.append(val_loss)

    epoch += 1

    plt.subplot(2,1,1)
    plt.plot(train_losses, alpha=0.7)
    plt.yscale('log')
    plt.ylabel('loss')

    plt.subplot(2,1,2)
    plt.plot(val_losses)
    plt.yscale('log')
    plt.xlabel('epoch')
    plt.ylabel('loss')

    plt.tight_layout()
    plt.savefig('loss.png')
    plt.close()

    torch.save({
        "model": model.state_dict(),
        "optimizer": optimizer.state_dict(),
        "scheduler": scheduler.state_dict(),
        "epoch": epoch
    }, f"checkpoint{epoch+1}_val{val_loss:.6f}.pt")
