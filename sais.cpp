#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

template<class T>
int inducedSort(std::vector<T> &M, std::vector<size_t> &lms_vector, size_t alphabet_size, std::vector<size_t> &sa) {
  M.emplace_back(static_cast<T>(0)); // 終端文字挿入
  int n = M.size();
  std::vector<bool> LS(n, false);  // L: false, S: true;
  std::vector<bool> LMS(n, false); // LMS: true

  std::vector<int> char_count(alphabet_size, 0);  // 各文字の出現回数（idxが文字ID）


  for (int ls_idx = n - 1; ls_idx >= 0; ls_idx--) {
    if (ls_idx == n - 1 || M[ls_idx] < M[ls_idx + 1]){
      LS[ls_idx] = true;
    } else if (M[ls_idx] == M[ls_idx + 1]) {
      LS[ls_idx] = LS[ls_idx + 1];
    } else if (LS[ls_idx + 1] == true) {
      LMS[ls_idx + 1] = true;
    }
    char_count[static_cast<size_t>(M[ls_idx])]++;
  }

  /* バケットソートのバケツとして使用する（idxは先頭アルファベット） */
  std::vector<std::vector<int> > bucket;
  bucket.resize(alphabet_size);

  /* 各バケット内vectorのinsertするidx（idxは先頭アルファベット） */
  std::vector<int> backet_pos_vec(alphabet_size, 0);
  /* あらかじめ各種文字の出現数分の固定長を用意する */
  for (int c = 0; c < alphabet_size; c++) {
    if (char_count[c] == 0) continue; 
    bucket[c].resize(char_count[c], -1);  // 未埋部分は-1を入れる
    backet_pos_vec[c] = char_count[c] - 1;  // 末尾から詰め込む
  }
  /* すでにバケット内部に書き込み済みか否かを示すフラグ */
  std::vector<bool> done(n, false);
  /* LMSをバケット内vectorの後ろから詰める */ 
  if (lms_vector.size() == 0) {
    for (int ls_idx = 0; ls_idx < n; ls_idx++) {
      if (LMS[ls_idx] == false) continue;
      size_t c = static_cast<size_t>(M[ls_idx]);
      bucket[c][backet_pos_vec[c]--] = ls_idx;
      done[ls_idx] = true;
    }
  } else {
    /* 末尾から詰め込んでいるので逆順で入れる */
    for (int lms_idx = lms_vector.size() - 1; lms_idx >= 0; lms_idx--) {
      int ls_idx = lms_vector[lms_idx];
      size_t c = static_cast<size_t>(M[ls_idx]);
      bucket[c][backet_pos_vec[c]--] = ls_idx;
      done[ls_idx] = true;
    }
  }

  /* Lをバケット内vectorの先頭から詰める */
  for (int c = 0; c < alphabet_size; c++) {
    /* 詰め込む箇所を先頭に変更 */
    backet_pos_vec[c] = 0;
  }

  for (int sm_idx = 0; sm_idx < alphabet_size; sm_idx++) {
    for (int bucket_inner_idx = 0; bucket_inner_idx < char_count[sm_idx]; bucket_inner_idx++) {
      int ls_idx = bucket[sm_idx][bucket_inner_idx];
      if (ls_idx == -1) continue; //  未埋部分
      if (ls_idx != 0 && LS[ls_idx - 1] == false && done[ls_idx - 1] == false) {
        size_t bucket_idx = static_cast<size_t>(M[ls_idx - 1]);
        bucket[bucket_idx][backet_pos_vec[bucket_idx]++] = ls_idx - 1;
        done[ls_idx - 1] = true;
      }
      /* LMSなら削除(終端記号はLMSだが残す) */
      if (LMS[ls_idx] == true && ls_idx != n - 1) {
        bucket[sm_idx][bucket_inner_idx] = -1;
        done[ls_idx] = false;
      }
    }
  }

  /* Lの後にSを挿入する */
  for (int c = 0; c < alphabet_size; c++) {
    /* 詰め込む箇所を末尾に変更 */
    backet_pos_vec[c] = char_count[c] - 1;
  }
  for (int sm_idx = alphabet_size - 1; sm_idx >= 0; sm_idx--) {
    for (int bucket_inner_idx = char_count[sm_idx] - 1; bucket_inner_idx >= 0; bucket_inner_idx--) {
      int ls_idx = bucket[sm_idx][bucket_inner_idx];
      if (ls_idx == -1) continue; //  未埋部分
      if (ls_idx != 0 && LS[ls_idx - 1] == true && done[ls_idx - 1] == false) {
        size_t bucket_idx = static_cast<size_t>(M[ls_idx - 1]);
        bucket[bucket_idx][backet_pos_vec[bucket_idx]--] = ls_idx - 1;
        done[ls_idx - 1] = true;
      }
    }
  }

  /* LMS部分文字列がソート済みの場合 */
  sa.reserve(n);
  if (lms_vector.size() > 0) {
    for (int sm_idx = 0; sm_idx < alphabet_size; sm_idx++) {
      for (auto ls_idx : bucket[sm_idx]) {
        if (ls_idx == n - 1) {  // 終端文字削除
          continue;
        }
        sa.emplace_back(ls_idx);
      }
    }
    return 0;
  }

  /* LMS部分文字列に重複がある場合は再起でソートする */
  std::vector<std::pair<int, int> > lms_uniq; // LMSの異なりを記録する(出現位置, 長さ)
  lms_uniq.reserve(n);
  bool lms_dedup_flag = false;
  std::vector<size_t> lms_bucket;
  lms_bucket.resize(n);
  for (int sm_idx = 0; sm_idx < alphabet_size; sm_idx++) {
    for (auto ls_idx : bucket[sm_idx]) {
      if (LMS[ls_idx] == true) {
        int lms_len = 1;
        if (ls_idx != n - 1) {
          while (LMS[ls_idx + lms_len] == false) {
            lms_len++;
          }
          lms_len++;
        }
        // 重複があるか否か（＝一個前のLMS部分文字列と一致するか否か）
        if (lms_uniq.size() == 0) {
          lms_uniq.emplace_back(std::make_pair(ls_idx, lms_len));
        } else {
          auto prev = lms_uniq.back();
          if (lms_len == prev.second) {
            bool uniq_flag = false;
            for (int cmp_idx = 0; cmp_idx < lms_len; cmp_idx++) {
              if (M[ls_idx + cmp_idx] == M[prev.first + cmp_idx]) continue;
              uniq_flag = true;
              break;
            }
            if (uniq_flag == false) {
              lms_dedup_flag = true;
            } else {
              lms_uniq.emplace_back(std::make_pair(ls_idx, lms_len));
            }
          } else {
            lms_uniq.emplace_back(std::make_pair(ls_idx, lms_len));
          }
        }
        lms_bucket[ls_idx] = lms_uniq.size(); // LMS部分文字列に割り当てるIDは1️オリジンとし、0は空けておく
      }
    }
  }

  std::vector<size_t> correct_lms_vector;
  correct_lms_vector.reserve(n / 2);
  std::vector<int> replaced_M;
  replaced_M.reserve(n / 2);
  std::vector<size_t> lms_vec;
  lms_vec.reserve(n / 2);
  for (size_t lms_bucket_idx = 0, lms_bucket_end = lms_bucket.size(); lms_bucket_idx < lms_bucket_end; lms_bucket_idx++) {
    size_t lms_id = lms_bucket[lms_bucket_idx];
    if (lms_id == 0) continue;  // bucketは空
    replaced_M.emplace_back(lms_id);
    lms_vec.emplace_back(lms_bucket_idx);
  }
  if (lms_dedup_flag) {
    std::vector<size_t> replaced_sa;
    std::vector<size_t> dummy;
    inducedSort(replaced_M, dummy, lms_uniq.size() + 1, replaced_sa);
    for (auto replaced_M_idx : replaced_sa) {
      correct_lms_vector.emplace_back(lms_vec[replaced_M_idx]);
    }
  } else {
    std::vector<int> r_M_bucket;
    r_M_bucket.resize(replaced_M.size() + 1, 0);
    r_M_bucket[0] = -1; // 0は使わなかったので無効
    for (int r_idx = 0; r_idx < replaced_M.size(); r_idx++) {
      r_M_bucket[replaced_M[r_idx]] = r_idx;
    }
    
    for (auto r_idx: r_M_bucket) {
      if (r_idx == -1) continue;
      correct_lms_vector.emplace_back(lms_vec[r_idx]);
    }
  }
  M.pop_back();  // 終端文字削除
  inducedSort(M, correct_lms_vector, alphabet_size, sa);
  return 0;
}

template<class T>
int sais(std::vector<T> &M, size_t alphabet_size, std::vector<size_t> &sa) {
  std::vector<size_t> dummy;
  inducedSort(M, dummy, alphabet_size, sa);
  return 0;
}

template <class T>
int lcp_kasai (std::vector<T> &M, std::vector<size_t> &sa, std::vector<int> &LCP) {
  M.emplace_back(static_cast<T>(0)); // 終端文字挿入
  int n = M.size(); 

  LCP.clear();
  LCP.resize(n, 0);

  std::vector<int> rank(n, 0);

  for (size_t sa_idx = 0; sa_idx < sa.size(); sa_idx++) {
    rank[sa[sa_idx]] = sa_idx;
  }

  int l = 0;
  for (int i = 0; i < n; i++) {
    int i1 = i;
    int i2 = sa[rank[i] - 1];
    while (i1 + l < n && i2 + l < n && M[i1 + 1] == M[i2 + 1]) {
      l++;
    }
    LCP[rank[i]] = l;
    if (l > 0) {
      l--;
    }
  } 
  M.pop_back();  // 終端文字削除
  return 0;
}

template <class T>
int plcp_phi_inplace (std::vector<T> &M, std::vector<size_t> &sa, std::vector<int> &Phi) {
  M.emplace_back(static_cast<T>(0)); // 終端文字挿入
  int n = M.size(); 

  Phi.clear();
  Phi.resize(n, 0);
  for (size_t sa_idx = 0; sa_idx < sa.size(); sa_idx++) {
    Phi[sa[sa_idx]] = sa[sa_idx - 1];
  }

  int l = 0;
  for (int i = 0; i < n; i++) {
    int j = Phi[i] - 1;
    while (i + l < n && j + l < n && M[i + 1] == M[j + 1]) {
      l++;
    }
    Phi[i] = l;
    if (l > 0) {
      l--;
    }
  } 


  return 0;
}

int main() {
  std::ifstream ifs("./tmp.txt");
  std::string s;
  std::string line;
  while (getline(ifs, line)) {
    for (char c: line) {
      if (c != ' ' && c != '\t') {
        s.push_back(c);
      }
    }
  }

  // std::string s = "ioioioioioioioioioioioioioio";

  std::vector<unsigned char> array;
 
  for (auto i:s) {
    array.push_back((unsigned char) i);
  }
  std::vector<size_t> sa;
  sais(array, 256, sa);
  int count = 0;
  for (auto text_idx : sa) {
    std::cout << text_idx << ":" << s.substr(text_idx, 50) << std::endl;;
    if (count++ > 100) break;
  }

  // std::string T;
  // std::cin >> T;
  // int Q;
  // std::cin >> Q;
  // std::vector<std::string> P;
  // for (int i = 0; i < Q; i++) {
  //   std::string s;
  //   std::cin >> s;
  //   P.push_back(s);
  // }

  // std::vector<unsigned char> array;
  // for (char i: T) {
  //   array.push_back((unsigned char) i);
  // }
  // std::vector<size_t> sa;
  // sais(array, 256, sa);
  // // for (auto text_idx : sa) {
  // //   std::cout << text_idx << ":" << T.substr(text_idx, 50) << std::endl;;
  // // }

  // for (auto t: P) {
  //   int L = 0;
  //   int R = sa.size();
  //   while(R - L > 1) {
  //     int M = (L + R) >> 1;
  //     if (T.substr(sa[M], t.size()) <= t) {
  //       L = M;
  //     } else {
  //       R = M;
  //     }
  //   }
  //   std::cout << (T.substr(sa[L], t.size()) == t) << std::endl;
  // }

  return 0;
}