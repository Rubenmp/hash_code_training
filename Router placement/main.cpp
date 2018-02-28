#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

///////////////
// VARIABLES //
///////////////

int h, // rows of the grid
  w, // columns of the grid
  r, // radius of a router range
  pr, // Placing a single router costs
  pb, // Connecting a single cell to the backbone costs
  b, // maximum spend on routers and backbone
  current_b=0;

const int N_MAX = 1000;
enum Type { wall,   // #
            target, // .
            avoid   // -
};

Type building[N_MAX][N_MAX];
int cumulative_walls[N_MAX][N_MAX]; // Walls in the rectangle with corners [0,0] and [x,y]
int routers[N_MAX][N_MAX];
int connections[N_MAX][N_MAX];
int signal[N_MAX][N_MAX];
struct Point{
  int x,y;
  Point(int a, int b):x(a),y(b){}
  Point():x(-1),y(-1){}
};
struct Path{
  Point initial, end;
  int cost;
  bool possible;
  std::vector<Point> points;
  Path(Point a, Point b):initial(a),end(b){
    // Make path between two points
    int increase_x, increase_y;
    possible = true;

    if ((end.x-initial.x) >= 0) increase_x = 1;
    else  increase_x = -1;
    if ((end.y-initial.y) >= 0) increase_y = 1;
    else  increase_y = -1;


    int minimum = min(abs(initial.x-end.x), abs(initial.y-end.y));
    bool connected = false;
    int i=0;
    Point next(initial.x, initial.y);
    while(!connected){
      if (i < minimum){
        next.x += increase_x;
        next.y += increase_y;
      }
      else{ // straight path
        if (abs(initial.x-end.x) > minimum)
          next.x += increase_x;
        else if (abs(initial.y-end.y) > minimum)
          next.y += increase_y;
      }
      if (connections[next.x][next.y]){
        possible = false;
        break;
      }
      else{
        points.push_back(next);
        connections[next.x][next.y] = 1;
        if (next.x == end.x and next.y == end.y)
          connected = true;
      }

      ++i;
    }

    cost = i*pb + pr;
  }
  Path(){}
  ~Path(){
    if (points.size() > 0){
      for (auto p : points)
        connections[p.x][p.y] = 0;
    }
  }

};
Point backbone;
const int NPATHS = 4;
std::vector<Path> paths[NPATHS];


///////////////
// FUNCTIONS //
///////////////
bool is_covered(Point router, Point other);
double price_to_connect(Point backbone, Point router);
int cells_to_connect(Point backbone, Point router);
int exists_wall(Point one, Point two);
int increase_signal(Point p);
void place_router(Point p);



bool is_covered(Point router, Point other){
  // Solve if 'router' signal cover point 'other'
  bool cov = true;
  if (abs(router.x-other.x) > r) cov = false;
  if (abs(router.y-other.y) > r) cov = false;
  if (exists_wall(router, other)) cov = false;
  return cov;
}
double price_to_connect(Point backbone, Point router){
  // Efficiency: constant
  int minimum = min(abs(backbone.x-router.x), abs(backbone.y-router.y));
  int cells = minimum;
  if (abs(backbone.x-router.x) > minimum)
    cells += (abs(backbone.x-router.x) - minimum);
  if (abs(backbone.y-router.y) > minimum)
    cells += (abs(backbone.y-router.y) - minimum);
  return (cells*pb +pr);
}
int cells_to_connect(Point backbone, Point router){
  // Efficiency: constant
  int minimum = min(abs(backbone.x-router.x), abs(backbone.y-router.y));
  int cells = minimum;
  if (abs(backbone.x-router.x) > minimum)
    cells += (abs(backbone.x-router.x) - minimum);
  if (abs(backbone.y-router.y) > minimum)
    cells += (abs(backbone.y-router.y) - minimum);
  return (cells);
}
int exists_wall(Point one, Point two){
  // Return: Is there any wall inside the smallest enclosing rectangle of two points?
  // Efficiency: constant

  Point left_up, right_down;
  left_up.x = min(one.x, two.x);
  left_up.y = min(one.y, two.y);
  right_down.x = max(one.x, two.x);
  right_down.y = max(one.y, two.y);

  int walls = cumulative_walls[right_down.x][right_down.y];
  if (left_up.y >= 1)
    walls -= cumulative_walls[right_down.x][left_up.y-1];

  if (left_up.x >= 1)
    walls -= cumulative_walls[left_up.x-1][right_down.y];

  if (left_up.x >= 1 and left_up.y >= 1)
    walls += cumulative_walls[left_up.x-1][left_up.y-1];

  return(walls);
}
int increase_signal(Point p){
  // how many cells of signal we would obtain placing a router at p
  int inc = 0;
  for (int i=-r; i<=r; ++i){
    for (int j=-r; j<=r; ++j){
      int x = p.x+i, y = p.y+j;

      if (x>=0 and y>=0 and x<=h and y<=w){
        if (building[x][y] == target and !signal[x][y]){
          if (exists_wall(p, Point(x,y)) == false)
            ++inc;
        }
      }
    }
  }
  return(inc);
}
void place_router(Point p){
  // Specify signal changes in global variable 'signal'
  if (building[p.x][p.y] == wall)
    cerr << "Trying to put a router in wall" << endl;
  else{
    routers[p.x][p.y] = 1;
    for (int i=-r; i<=r; ++i){
      for (int j=-r; j<=r; ++j){
        int x = p.x+i;
        int y = p.y+j;

        if (x>=0 and y>=0 and x<=h and y<=w){
          if (building[x][y] == target and (!exists_wall(p,Point(x,y))))
            signal[x][y] = 1;
        }
      }
    }
  }
}

void precomp() {
  // Cumulative_walls
  for (int i=0,j=0; i<h && j<w; i++,j++)
    cumulative_walls[i][j] = 0;
  for (int i=0; i<h; i++){
    if (building[i][0] == wall)
      cumulative_walls[i][0] = 1;
  }
  for (int j=0; j<w; ++j){
    if (building[0][j] == wall)
      cumulative_walls[0][j] = 1;
  }
  for (int i=1; i<h; i++){
    for (int j=1; j<w; j++){
      cumulative_walls[i][j] = cumulative_walls[i-1][j] + cumulative_walls[i][j-1] - cumulative_walls[i-1][j-1];
      if (building[i][j] == wall)
        ++cumulative_walls[i][j];
    }
  }

  // Routers
  for (int i=0,j=0; i<h and j<w; i++,j++){
    routers[i][j] = 0;
    connections[i][j] = 0;
    signal[i][j] = 0;
  }
}

void input() {
  char aux;
  cin >> h >> w >> r;
  cin >> pb >> pr >> b;
  cin >> backbone.x >> backbone.y;
  for (int i=0; i<h; ++i){
    for (int j=0; j<w; ++j){
      cin >> aux;
      if (aux == '#')
        building[i][j] = wall;
      else if (aux == '.')
        building[i][j] = target;
      else if (aux == '-')
        building[i][j] = avoid;
      else
        cerr << "Error in input" << endl;
    }
  }
}

void sol() {
  bool finish = false;
  /*bool used[NPATHS], first_routers_placed = false;

  int increase[NPATHS][2];
  increase[0][0] = -1;
  increase[0][1] = -1;
  increase[1][0] = -1;
  increase[1][1] = 1;
  increase[2][0] = 1;
  increase[2][1] = -1;
  increase[3][0] = 1;
  increase[3][1] = 1;
  */

  Point previous_r(backbone.x, backbone.y);
  Point next_router;
  Path next_path;
  int new_signal, max_signal;

  while (!finish){
    max_signal = 0;
    for (int i=-r,j=-r; i<=r and j<=r; ++i,++j){
      Point p(previous_r.x + i, previous_r.y + j);

      if (building[p.x][p.y] != wall){
        new_signal = increase_signal(p);
        //cerr << new_signal << endl;
        if (new_signal > max_signal){
          next_router.x = previous_r.x + i;
          next_router.y = previous_r.y + j;
          Path path(previous_r, next_router);

          if (path.possible and (path.cost <= (b-current_b)) ){
            next_router = p;
            next_path = path;
            max_signal = new_signal;
          }
        }


      }
    }
    if (max_signal == 0)
      finish = true;
    else{
      place_router(next_router);
      previous_r = next_router;
      paths[0].push_back(next_path);
    }

  }
}

int score(){
  // not finish
  int score = b-current_b;

  std::vector<Point> p_routers;
  std::vector<Point> p_connections;
  for (int i=0; i<h; i++){
    for(int j=0; j<w; j++){
      if (routers[i][j])
        p_routers.push_back(Point(i,j));
    }
  }

}

void debug() {
  /*cerr << h << " " << w << " " << r << endl;
  cerr << pb << " " << pr << " " << b << endl;
  cerr << backbone.x << " " << backbone.y << endl;
  for (int i=0; i<h; i++){
    for (int j=0; j<w; j++)
        cerr << building[i][j];
      cerr << endl;
  }
  */
}

void print_sol() {
  std::vector<Point> p_routers;
  std::vector<Point> p_connections;
  for (int i=0; i<h; i++){
    for(int j=0; j<w; j++){
      if (routers[i][j])
        p_routers.push_back(Point(i,j));
    }
  }

  // print p_connections
  int total = 0;
  for (auto s : paths[0])
    total += s.points.size();

  cout << total << endl;
  for (auto p : paths[0]){
    for (auto s : p.points)
      cout << s.x << " " << s.y << endl;
  }

  // print Routers
  cout << p_routers.size() << endl;
  for (auto p : p_routers)
    cout << p.x << " " << p.y << endl;

}

signed main(int argc, char *argv[]) {
  // Files: https://hashcode.withgoogle.com/past_editions.html
  const char * file_in = argv[1];
  const char * file_out = argv[2];

  srand(unsigned(std::time(0)));

  freopen(file_in, "r", stdin);
  freopen(file_out, "w", stdout);



  // Start computations
  input();
  precomp();
  // debug();
  sol();
  print_sol();
  // End computations

  return 0;
}
