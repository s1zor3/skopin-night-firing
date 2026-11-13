#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>
#include <unordered_set>
namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    // 1. Слева направо вычеркнуть duplicate, затем invalid, затем thick.
    //    Вычеркнутые не стоят в очереди и не рвут садку.
    // 2. Пока кто-то ждёт, собрать одну садку по правилам README.
    //    На стопку сажать с конца к началу: первое взятое снимается первым.
    // 3. Провести её объектом Kiln. После утиля, за которым сразу полива,
    //    горн не остужать. stokes взять из kiln.stokes() в конце.
    NightReport report;
    std::unordered_set<std::string> repeat_marks;
    std::vector<Ware> valid_queue;
    for (const Ware& el : queue) {
        if (repeat_marks.count(el.mark)) {
            report.refused.push_back({el.mark, "duplicate"});
        } else {
            repeat_marks.insert(el.mark);
            if (validateWare(el).has_value()) {
                report.refused.push_back({el.mark, "invalid"});
            } else if (el.thickness_mm > kCrackThicknessMm) {
                report.refused.push_back({el.mark, "thick"});
            } else {
                valid_queue.push_back(el);
            }
        }
    }
    
    std::vector<std::vector<Ware>> batches;
    std::vector<Ware> remaining = valid_queue;
    while (!remaining.empty()) {
        std::vector<Ware> current_batch;
        std::vector<Ware> next_remaining;
        Ware base = remaining[0];
        current_batch.push_back(base);
        if (base.kind == WareKind::Sagger) {
            for (size_t i = 1; i < remaining.size(); ++i) {
                next_remaining.push_back(remaining[i]);
            }
        } else {
            Program P = base.program;
            bool skipped = false;
            bool wall = false;
            for (size_t i = 1; i < remaining.size(); ++i) {
                const Ware& candidate = remaining[i];
                if (wall) {
                    next_remaining.push_back(candidate);
                    continue;
                }
                if (candidate.program != P || candidate.kind == WareKind::Sagger) {
                    wall = true;
                    next_remaining.push_back(candidate);
                    continue;
                }
                
                bool fits = fitsInKiln(current_batch, candidate);
                bool can_take = fits && (!skipped || candidate.kind == WareKind::Rush);
                if (can_take) {
                    current_batch.push_back(candidate);
                } else {
                    skipped = true;
                    next_remaining.push_back(candidate);
                }
            }
        }
        batches.push_back(current_batch);
        remaining = next_remaining;
    }

    Kiln kiln;

    for (size_t b_idx = 0; b_idx < batches.size(); ++b_idx) {
        const auto& batch = batches[b_idx];
        Program P = batch[0].program;

        if (P == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();
            for (int i = static_cast<int>(batch.size()) - 1; i >= 0; --i) {
                kiln.load(batch[i]);
            }
            kiln.close();

            kiln.fire();

            kiln.open();
            for (size_t i = 0; i < batch.size(); ++i) {
                kiln.unload(batch[i].mark);
                report.passed.push_back(batch[i].mark);
            }
            bool next_is_glaze = (b_idx + 1 < batches.size()) && (batches[b_idx + 1][0].program == Program::Glaze);
            if (!next_is_glaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }
            for (int i = static_cast<int>(batch.size()) - 1; i >= 0; --i) {
                kiln.load(batch[i]);
            }
            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
                kiln.fire();
            } else {
                kiln.fire();
            }
            kiln.cool();
            kiln.cool();
            kiln.open();
            for (size_t i = 0; i < batch.size(); ++i) {
                kiln.unload(batch[i].mark);
                report.passed.push_back(batch[i].mark);
            }
            kiln.close();
        }
    }

    report.stokes = kiln.stokes();

    return report;
}

}  // namespace skopin
