#ifndef UTILS_H_
#define UTILS_H_

#include <ssw/ssw_cpp.h> // Striped Smith-Waterman aligner
#include <io/read.hpp>
#include "additional.cpp"
#include "running_modes.hpp"
#include "adapter_index.hpp"

namespace cclean_utils {

std::string ReverseComplement(const std::string& read);

double GetScoreWithQuality(const StripedSmithWaterman::Alignment &a,
                                            const Quality &qual);

// Cut read from start to end position of best aligment with adapter
Read CutRead(const Read &r, int start_pos, int end_pos);
void RestoreFromCigar(const std::string& ref, const std::string& query,
                      std::string& out_ref, std::string& out_query,
                      const StripedSmithWaterman::Alignment& a);

inline int GetMismatches(const std::string &read, const std::string &adapter,
                         const StripedSmithWaterman::Alignment &a)  {
  std::string aligned_read;
  std::string aligned_adapter;
  RestoreFromCigar(read, adapter, aligned_read, aligned_adapter, a);
  int size = (int)std::min(aligned_read.length(), aligned_adapter.length());
  int mismatched_score = 0;
  for (int i = 0; i < size; ++i)  {
    if (aligned_read[i] != aligned_adapter[i])
      ++mismatched_score;
  }
  return mismatched_score;
}

std::unordered_map<std::string, std::string> ProcessArgs(int argc, char *argv[],
                                                         bool *ok, std::string *error);

// end of namespace
}
#endif /* UTILS_H_ */
