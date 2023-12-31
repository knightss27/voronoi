#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct position {
  int x;
  int y;
} position;

typedef struct pixel {
  position seed;
  unsigned char r;
  unsigned char g;
  unsigned char b;
} pixel;

typedef struct image {
  int width;
  int height;
  pixel *pixels;
} image;

// writes ppm image to stdout (either P3 or P6)
void write_to_ppm(image img, int use_p6) {
  printf("P%d\n%d %d\n255\n", use_p6 ? 6 : 3, img.width, img.height);
  for (int i = 0; i < img.width * img.height; i++) {
    pixel p = img.pixels[i];
    if (use_p6) {
      printf("%c%c%c", p.r, p.g, p.b);
    } else {
      printf("%d %d %d\n", p.r, p.g, p.b);
    }
  }
}

// sets the seed pixels to random colors in mildly random places
void set_seed_pixels(image img) {

  // n * n = number of seeds
  int n = 10;

  for (int x = img.width / n; x < img.width; x += img.width / n) {
    for (int y = img.height / n; y < img.height; y += img.height / n) {
      int nx = rand() % 2;
      int ny = rand() % 2;
      if (nx)
        nx = -nx;
      if (ny)
        ny = -ny;
      nx = x + (nx * (rand() % (img.width / n - 1)));
      ny = y + (ny * (rand() % (img.height / n - 1)));

      img.pixels[nx + ny * img.width].r = rand() % 255;
      img.pixels[nx + ny * img.width].g = rand() % 255;
      img.pixels[nx + ny * img.width].b = rand() % 255;
      img.pixels[nx + ny * img.width].seed = (position){nx, ny};
    }
  }
}

// returns the un-sqrted euclidean distance
unsigned int sdist(position p1, position p2) {
  return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

// runs the jump flooding algorithm on the seed image
void jfa(image img) {
  int kd = 1;
  while (kd <= img.width) {
    // technically we run 1+JFA
    int k = kd == 1 ? 1 : img.width / kd;

    // main stuff
    for (int y = 0; y < img.height; y++) {
      for (int x = 0; x < img.width; x++) {

        pixel p = img.pixels[x + y * img.width];

        // neighbors
        for (int i = -k; i <= k; i += k) {
          if (x + i < 0 || x + i >= img.width) {
            continue;
          }

          for (int j = -k; j <= k; j += k) {
            if (y + j < 0 || y + j >= img.height) {
              continue;
            }

            pixel q = img.pixels[(x + i) + (y + j) * img.width];

            // if you have different colors, change the current pixel
            if (p.r == 0 && p.g == 0 && p.b == 0 &&
                (q.r != 0 || q.g != 0 || q.b != 0)) {
              img.pixels[x + y * img.width].r = q.r;
              img.pixels[x + y * img.width].g = q.g;
              img.pixels[x + y * img.width].b = q.b;
              img.pixels[x + y * img.width].seed = q.seed;
            }

            // reassign p to keep up to date
            p = img.pixels[x + y * img.width];

            // if you have the same colors (and they aren't black)
            if ((p.r != 0 || p.g != 0 || p.b != 0) &&
                (q.r != 0 || q.g != 0 || q.b != 0)) {
              position ppos = (position){x, y};

              // if the distance to the other seed is closer, swap seeds
              if (sdist(ppos, p.seed) > sdist(ppos, q.seed)) {
                img.pixels[x + y * img.width].r = q.r;
                img.pixels[x + y * img.width].g = q.g;
                img.pixels[x + y * img.width].b = q.b;
                img.pixels[x + y * img.width].seed = q.seed;
              }
            }
          }
        }
      }
    }

    kd *= 2;
  }
}

// check if the current position is in the given position array
int check_seen_pos(position ps[], position s) {
  for (int i = 0; i < 8; i++) {
    if (ps[i].x == s.x && ps[i].y == s.y) {
      return 1;
    }
  }
  return 0;
}

void make_borders(image img) {
  // for every pixel
  for (int y = 1; y < img.height - 1; y++) {
    for (int x = 1; x < img.width - 1; x++) {

      int is_border = 0;

      position ps[8] = {0};
      ps[0] = img.pixels[x + y * img.width].seed;

      int next_new_i = 1;

      // check the borders
      for (int i = -1; i <= 1; i += 1) {
        for (int j = -1; j <= 1; j += 1) {

          pixel p = img.pixels[x + y * img.width];
          pixel q = img.pixels[(x + i) + (y + j) * img.width];

          // if we find the border
          if (p.seed.x != q.seed.x || p.seed.y != q.seed.y) {
            img.pixels[x + y * img.width].r = 255;
            img.pixels[x + y * img.width].g = 255;
            img.pixels[x + y * img.width].b = 255;

            is_border += 1;
            if (!check_seen_pos(ps, q.seed)) {
              ps[next_new_i] = q.seed;
              next_new_i++;
            }
          }
        }
      }

      // if not a border, make the pixel black
      if (!is_border) {
        img.pixels[x + y * img.width].r = 0;
        img.pixels[x + y * img.width].g = 0;
        img.pixels[x + y * img.width].b = 0;
      }

      // if there are 3 or more unique seeds around this pixel, it's a corner
      if (next_new_i > 2) {
        img.pixels[x + y * img.width].r = 251;
        img.pixels[x + y * img.width].g = 72;
        img.pixels[x + y * img.width].b = 196;
      }
    }
  }
}

int main() {

  // image initializaiton
  image img;
  img.width = 2000;
  img.height = 2000;
  img.pixels = calloc(img.width * img.height, sizeof(pixel));

  // ...
  srand(time(NULL));

  // initialize seed locations
  set_seed_pixels(img);

  // run the jump flooding algorithm
  jfa(img);

  // add white borders / black fill
  make_borders(img);

  // write P6 to stdout
  write_to_ppm(img, 1);

  // good practice
  free(img.pixels);
  return 0;
}
