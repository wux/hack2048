

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <iomanip>
#include <string>
#include <sys/time.h>
using namespace std;
// Start
//  4, 0, 2, 64,
//  0, 4, 4, 8,
//  0, 32, 512, 4096,
//  4, 16, 16, 2048,


static int M[4 * 4] = {
  4, 2, 64, 0,
  8, 8, 0, 2,
  32, 512, 4096, 0,
  4, 32, 2048, 0
};


// Static internal buffer for all moves/rot functions.
static int A[16];
static int B[16];

void print(int m[16]) {
  cout << endl;
  for (int* r = m; r < m + 16; r += 4) {
    for (int* c = r; c < r + 4; ++c) {
      cout << right << setw(4) << *c << ", ";
    }
    cout << endl;
  }
}

int* move_left(int in[16]) {
  int* out = in == A ? B : A;
  memcpy(out, in, 16 * sizeof(int));

  for (int* r = out; r < out + 16; r +=4) {
    int* c = r;
    while (c < r + 3) {
      if (*c == 0) {
	++c;
      } else {
	int* t = c + 1;
	while(*t == 0 && t < r + 4) 
	  ++t;
	if (t < r + 4 && *t == *c) {
	  *c *= 2;
	  *t = 0;
	  c = t;
	} else {
	  ++c;
	}
      }
    }
  }

  for (int* r = out; r < out + 16; r +=4) {
    int* t = r;
    for (int* c = r; c < r + 4; ++c) {
      while (*t == 0 && t < r + 4) {
	t++;
      }
      *c = t < r + 4 ? *t : 0;
      t++;
    }
  }

  return out;
}

int* move_right(int in[16]) {
  int* out = in == A ? B : A;
  memcpy(out, in, 16 * sizeof(int));

  for (int* r = out; r < out + 16; r +=4) {
    int* c = r + 3;
    while (c >= r) {
      if (*c == 0) {
	--c;
      } else {
	int* t = c - 1;
	while (*t == 0 && t >= r)
	  t--;
	if (t >= r && *t == *c) {
	  *c *= 2;
	  *t = 0;
	  c = t;
	} else {
	  c--;
	}
      }
    }
  }

  for (int* r = out; r < out + 16; r +=4) {
    int* t = r + 3;
    for (int* c = r + 3; c >= r; --c) {
      while (*t == 0 && t >= r)
	t--;
      *c = t >= r ? *t : 0;
      t--;
    }
  }

  return out;
}

int* ccw_rot(int in[16]) {
  int* out = in == A ? B : A;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c) 
      out[r * 4 + c] = in[c * 4 + 3 - r];
  return out;
}

int* left(int in[16]) {
  return move_left(in);
}

int* right(int in[16]) {
  return move_right(in);
}

int* down(int in[16]) {
  return ccw_rot(ccw_rot(ccw_rot(move_right(ccw_rot(in)))));
}

int* up(int in[16]) {
  return ccw_rot(ccw_rot(ccw_rot(move_left(ccw_rot(in)))));
}

uint64_t row_word(int in[16]) {
  uint64_t w = 0;
  for(int* p = in; p < in + 16;  p += 1) {
    uint64_t t = max(0, fls(*p) - 1);
    w |= t << ((15 - (p - in)) * 4);
    cout << t << ", " << std::hex << w << endl;
  }
  return w;
}

uint64_t col_word(int in[16]) {
  uint64_t w = 0;
  int i = 60;
  for(int* c = in; c < in + 4;  c += 1) {
    for (int* p = c; p < in + 16; p += 4) {
      uint64_t t = max(0, fls(*p) - 1);
      w |= t << i;
      i -= 4;
      cout << t << ", " << std::hex << w << endl;
    }
  }
  return w;
}

// <l><r><u><d>
int movable(int in[16]) {
  int rc = 0;
  int num_zeros = 0;
  for(int* p = in; p < in + 16;  ++p) {
    int* r = (p - in) % 4 == 3 ? nullptr : p + 1;
    int* l = (p - in) % 4 == 0 ? nullptr : p - 1;
    int* d = (p + 4 < in + 16) ? p + 4 : nullptr;
    int* u = (p - 4 >= in) ? p - 4 : nullptr;
    if (*p == 0) {
      num_zeros += 1;
      rc |= ((r && *r) << 3) | ((l && *l) << 2) |
	((d && *d) << 1) | (u && *u);
    } else {
      if (r && *r == *p)
	rc |= 12;
      if (d && *d == *p)
	rc |= 3;
    }
  }

  // luck hack!!
  if (num_zeros < 3)
    return 0;

  return rc;
}

int matrix_score(int in[16]) {
  int s = 0;
  for (int* p = in; p < in + 16; ++p)
    //s = max(s, *p);  // use max score.
    s += *p == 0 ? 4 : *p;
  //  cout << "matrix_score " << s << endl;
  return s;
}

int* (*moves[4])(int in[16]);

static int g_max;

// Running overnight and get path as long as 2991.
char dtree[4096];
int tail;

int best_move(int in[16]) {
  // Level control.
  if (tail > 4)
    return matrix_score(in);

  // Movable control.
  int move_flag = movable(in);
  if (!move_flag)
    return matrix_score(in);

  int max_score = 0;
  int move = -1;

  tail++;
  int m[16];
  for (int ii =0; ii < 4; ++ii) {
    if ((move_flag & (1 << (3 - ii))) == 0) {
      //      cout << std::hex << move_flag << " " << ii;
      continue;
    }
    int i = ii;
    int* internal_m = moves[i](in);
    memcpy(m, internal_m, 16 * sizeof(int));
    dtree[tail] = (char)i;

    int min_move_score = 4096 * 2 * 16;
    bool has_zero = false;
    for (int* p = m; p < m + 16; ++p) {
      if (*p != 0) {
	continue;
      }
      has_zero = true;
      *p = 2;      
      int score2 = best_move(m);
      if (score2 < max_score) {  // bad luck could kill this dir.
	min_move_score = 0;  // No need to check this move direction.
	break;
      }
      *p = 4;
      int score4 = best_move(m);
      int score = min(score2, score4);
      //  cout << score2 << "," << score4 << "," << score << "," << min_move_score;
      min_move_score = min(min_move_score, score);
      
      *p = 0;
    }
    
    if (!has_zero) {
      min_move_score = matrix_score(m);
    }
    assert(min_move_score != 4096 * 2 * 16);
    
    if (min_move_score > max_score) {
      move = i;
      max_score = min_move_score;
    }
    //cout << "best move " << move << " score " << max_score << endl;
    //    print(m);
  }
  //  cout << move << "_" << max_score << " : ";
  if (max_score > g_max) {
#if 0
    // Lucky loop.
    cout << g_max << "-->" << max_score << " move " << move << endl;
    print(in);
    for (int k = 0; k < tail; ++k) {
      switch ((int)dtree[k]) {
      case 0: cout << "L-"; break;
      case 1: cout << "R-"; break;
      case 2: cout << "U-"; break;
      case 3: cout << "D-"; break;
      default: assert(false);
      }
    }
    cout << endl;
#endif
    g_max = max_score;
  }
  tail--;

  if (tail == -1)
    cout << "BEST MOVE " << move << " Guranteed score " << max_score << endl;

  //  assert(move > -1);
  if (move == -1)
    return max_score;  // all dirs won't increase g_max.

  return max_score;
}

int main(int argc, char* argv[]) {

  moves[0] = left;
  moves[1] = right;
  moves[2] = up;
  moves[3] = down;
  g_max = -1;
  tail = -1;

  print(M);
  cout << " start score " << matrix_score(M) << endl;

#if TEST_RC_WORD
  //  uint64_t rw = col_word(M);
  // cout << std::hex << rw << endl;
#endif
#if TEST_MOVABLE
  cout << movable(M) << endl;
  for (int j = 0; j< 4; ++j) {
    int* m = moves[j](M);
    print(m);
    cout << movable(m) << endl;
  }
#endif

  struct timeval t0, t1;
  unsigned int i = 1;
  gettimeofday(&t0, NULL);

#if 0
  for(i = 0; i < 100; i++) {
    int s = best_move(M);
    // unit test for score = sum.
    // assert(s == 6826);
    // assert(g_max == 6826);
    // cout << endl << s;
  }
#else
  int s = best_move(M);
  cout << s << endl;
  
#endif

  gettimeofday(&t1, NULL);
  printf("Did %u calls in %.2g seconds\n", i, t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec));
}
