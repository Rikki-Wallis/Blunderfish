#include <torch/extension.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>

struct Record {
    std::vector<int32_t> features;
    float wdl, outcome;
};

Record parse_one(const uint8_t* ptr) {
    uint64_t bitboards[12];
    memcpy(bitboards, ptr, 12 * 8);
    int32_t score;   memcpy(&score,   ptr + 96, 4);
    int8_t  outcome; memcpy(&outcome, ptr + 101, 1);

    Record r;

    for (int side = 0; side < 2; side++) {
        for (int piece = 0; piece < 6; piece++)
        {
            uint64_t bb = bitboards[side*6 + piece];

            while (bb)
            {
                int sq = __builtin_ctzll(bb);
                bb &= bb - 1;
                r.features.push_back(side*6*64 + piece*64 + sq);
            }
        }
    }

    r.wdl     = 1.0f / (1.0f + expf(-(float)score / 400.0f));
    r.outcome = (float)outcome * 0.5f + 0.5f;

    return r;
}

// collate whole batch at once - returns pre-concatenated tensors
std::tuple<torch::Tensor, torch::Tensor>
collate_batch(const std::vector<std::string>& records, float cur_blend) {
    int B = records.size();

    std::vector<torch::Tensor> features;
    std::vector<float> targets;

    for (int i = 0; i < B; i++) {
        const uint8_t* ptr = (const uint8_t*)records[i].data();

        Record r = parse_one(ptr);

        float target = (1.0f - cur_blend) * r.wdl + cur_blend * r.outcome;

        while (r.features.size() < 32) {
            r.features.push_back(768);
        }

        features.push_back(torch::tensor(r.features));
        targets.push_back(target);
    }

    return {
        torch::stack(features),
        torch::tensor(targets,  torch::kFloat32)
    };
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("collate_batch", &collate_batch, "Collate batch of raw NNUE records");
}