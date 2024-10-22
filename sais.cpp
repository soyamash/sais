#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

template<class T>
int inducedSort(std::vector<T> &M, std::vector<size_t> &lms_vector, size_t alphabet_size, std::vector<size_t> &sa) {
  M.emplace_back(static_cast<T>(0)); // 終端文字挿入
  int M_len = M.size();
  std::vector<bool> LS(M_len, false);  // L: false, S: true;
  std::vector<bool> LMS(M_len, false); // LMS: true

  std::vector<int> char_count(alphabet_size, 0);  // 各文字の出現回数（idxが文字ID）


  for (int M_idx = M_len - 1; M_idx >= 0; M_idx--) {
    if (M_idx == M_len - 1 || M[M_idx] < M[M_idx + 1]){
      LS[M_idx] = true;
    } else if (M[M_idx] == M[M_idx + 1]) {
      LS[M_idx] = LS[M_idx + 1];
    } else if (LS[M_idx + 1] == true) {
      LMS[M_idx + 1] = true;
    }
    char_count[static_cast<size_t>(M[M_idx])]++;
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
  std::vector<bool> done(M_len, false);
  /* LMSをバケット内vectorの後ろから詰める */ 
  if (lms_vector.size() == 0) {
    for (int M_idx = 0; M_idx < M_len; M_idx++) {
      if (LMS[M_idx] == false) continue;
      size_t c = static_cast<size_t>(M[M_idx]);
      bucket[c][backet_pos_vec[c]--] = M_idx;
      done[M_idx] = true;
    }
  } else {
    /* 末尾から詰め込んでいるので逆順で入れる */
    for (int lms_idx = lms_vector.size() - 1; lms_idx >= 0; lms_idx--) {
      int M_idx = lms_vector[lms_idx];
      size_t c = static_cast<size_t>(M[M_idx]);
      bucket[c][backet_pos_vec[c]--] = M_idx;
      done[M_idx] = true;
    }
  }

  /* Lをバケット内vectorの先頭から詰める */
  for (int c = 0; c < alphabet_size; c++) {
    /* 詰め込む箇所を先頭に変更 */
    backet_pos_vec[c] = 0;
  }

  for (int sm_idx = 0; sm_idx < alphabet_size; sm_idx++) {
    for (int bucket_inner_idx = 0; bucket_inner_idx < char_count[sm_idx]; bucket_inner_idx++) {
      int M_idx = bucket[sm_idx][bucket_inner_idx];
      if (M_idx == -1) continue; //  未埋部分
      if (M_idx != 0 && LS[M_idx - 1] == false && done[M_idx - 1] == false) {
        size_t bucket_idx = static_cast<size_t>(M[M_idx - 1]);
        bucket[bucket_idx][backet_pos_vec[bucket_idx]++] = M_idx - 1;
        done[M_idx - 1] = true;
      }
      /* LMSなら削除(終端記号はLMSだが残す) */
      if (LMS[M_idx] == true && M_idx != M_len - 1) {
        bucket[sm_idx][bucket_inner_idx] = -1;
        done[M_idx] = false;
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
      int M_idx = bucket[sm_idx][bucket_inner_idx];
      if (M_idx == -1) continue; //  未埋部分
      if (M_idx != 0 && LS[M_idx - 1] == true && done[M_idx - 1] == false) {
        size_t bucket_idx = static_cast<size_t>(M[M_idx - 1]);
        bucket[bucket_idx][backet_pos_vec[bucket_idx]--] = M_idx - 1;
        done[M_idx - 1] = true;
      }
    }
  }

  /* LMS部分文字列がソート済みの場合 */
  sa.reserve(M_len);
  if (lms_vector.size() > 0) {
    for (int sm_idx = 0; sm_idx < alphabet_size; sm_idx++) {
      for (auto M_idx : bucket[sm_idx]) {
        if (M_idx == M_len - 1) {  // 終端文字削除
          continue;
        }
        sa.emplace_back(M_idx);
      }
    }
    return 0;
  }

  /* LMS部分文字列に重複がある場合は再起でソートする */
  bool lms_dedup_flag = false;  // LMS-substring重複フラグ
  std::vector<int> lms_bucket;  // LMSを出現位置（M_idx）順に並べ変えるためのバケット
  lms_bucket.resize(M_len, -1);
  std::pair<int, int> prev; // 一個前のLMS-substring（出現位置, 長さ）
  int lms_id = 0; // LMS部分文字列に割り当てるID
  std::vector<size_t> correct_lms_vector; // 重複無しの場合はSA中の順でLMSの正しい順序が得られる
  correct_lms_vector.reserve(M_len / 2);
  for (int sm_idx = 0; sm_idx < alphabet_size; sm_idx++) {
    for (auto M_idx : bucket[sm_idx]) {
      if (LMS[M_idx] == false) continue;
      int lms_len = 1;
      if (M_idx != M_len - 1) {
        while (LMS[M_idx + lms_len] == false) {
          lms_len++;
        }
        lms_len++;
      }
      // 重複があるか否か（＝一個前のLMS部分文字列と一致するか否か）
      if (lms_len == prev.second) {
        bool uniq_flag = false;
        for (int cmp_idx = 0; cmp_idx < lms_len; cmp_idx++) {
          if (M[M_idx + cmp_idx] == M[prev.first + cmp_idx]) continue;
          uniq_flag = true;
          break;
        }
        if (uniq_flag == false) {
          lms_dedup_flag = true;
        } else {
          prev.first = M_idx;
          prev.second = lms_len;
          correct_lms_vector.emplace_back(M_idx);
        }
      } else {
        prev.first = M_idx;
        prev.second = lms_len;
        correct_lms_vector.emplace_back(M_idx);
      }
      lms_bucket[M_idx] = ++lms_id; // 辞書順でIDを振る
    }
  }

  if (lms_dedup_flag) {
    std::vector<int> replaced_M;
    replaced_M.reserve(M_len / 2);
    std::vector<size_t> lms_M_idx_vec;
    lms_M_idx_vec.reserve(M_len / 2);
    for (size_t lms_bucket_idx = 0, lms_bucket_end = lms_bucket.size(); lms_bucket_idx < lms_bucket_end; lms_bucket_idx++) {
      if (lms_bucket[lms_bucket_idx] == -1) continue;  // bucketは空
      replaced_M.emplace_back(lms_bucket[lms_bucket_idx]);  // 出現順に並び替える（LMS-substringの結合文字列に対応）
      lms_M_idx_vec.emplace_back(lms_bucket_idx); // M_idxを保存
    }
    std::vector<size_t> replaced_sa;
    std::vector<size_t> dummy;
    inducedSort(replaced_M, dummy, lms_id + 1, replaced_sa);
    correct_lms_vector.clear();
    for (auto replaced_M_idx : replaced_sa) {
      correct_lms_vector.emplace_back(lms_M_idx_vec[replaced_M_idx]);
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
  int M_len = M.size(); 

  LCP.clear();
  LCP.resize(M_len, 0);

  std::vector<int> rank(M_len, 0);

  for (size_t sa_idx = 0; sa_idx < sa.size(); sa_idx++) {
    rank[sa[sa_idx]] = sa_idx;
  }

  int l = 0;
  for (int i = 0; i < M_len; i++) {
    int i1 = i;
    int i2 = sa[rank[i] - 1];
    while (i1 + l < M_len && i2 + l < M_len && M[i1 + 1] == M[i2 + 1]) {
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
  int M_len = M.size(); 

  Phi.clear();
  Phi.resize(M_len, 0);
  for (size_t sa_idx = 0; sa_idx < sa.size(); sa_idx++) {
    Phi[sa[sa_idx]] = sa[sa_idx - 1];
  }

  int l = 0;
  for (int i = 0; i < M_len; i++) {
    int j = Phi[i] - 1;
    while (i + l < M_len && j + l < M_len && M[i + 1] == M[j + 1]) {
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
  // std::ifstream ifs("./tmp.txt");
  // std::string s;
  // std::string line;
  // while (getline(ifs, line)) {
  //   for (char c: line) {
  //     if (c != ' ' && c != '\t') {
  //       s.push_back(c);
  //     }
  //   }
  // }

  std::string s = "ioioioioioioioio";

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