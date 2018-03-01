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
#include <fstream>
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
std::ostream& operator<<(std::ostream& os, const Point& p){
  os << "(" << p.x << "," << p.y << ")";
      return(os);
}
Point backbone;
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
      if (connections[next.x][next.y] or (next.x==backbone.x and next.y==backbone.y)){
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
std::ostream& operator<<(std::ostream& os, const Path& p){
  for (Point point : p.points)
    os << point << "-";
  return(os);
}
const int NPATHS = 400;
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
bool valid_point(Point p);


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
  int total = 0;
  for (int i=-r; i<=r; ++i){
    for (int j=-r; j<=r; ++j){
      total+=1;
      int x = p.x+i, y = p.y+j;

      if (valid_point(Point(x,y))){
        if (building[x][y] == target and !signal[x][y]){
          if (exists_wall(p, Point(x,y)) == false){
            ++inc;
          }
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
bool valid_point(Point p){
  return( p.x>=0 and p.x<h and p.y>=0 and p.y<w );
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

  Point previous_r;
  Point next_router;
  Path next_path;
  int new_signal, max_signal;

  int current_threat = 0;
  bool new_threat = true;
  while (!finish){
    bool change = false;

    if (new_threat){
      previous_r.x = backbone.x;
      previous_r.y = backbone.y;
      new_threat = false;
    }

    max_signal = 0;
    for (int i=-3*r; i<=3*r; ++i){
      for (int j=-3*r; j<=3*r; ++j){
        Point next(previous_r.x + i, previous_r.y + j);
        if (valid_point(next)){
          if (building[next.x][next.y] != wall){
            new_signal = increase_signal(next);
            if (new_signal > max_signal){
              Path path(previous_r, next);

              if (path.possible and (path.cost <= (b-current_b)) ){
                next_path = path;
                max_signal = new_signal;
                next_router = next;
                change = true;
              }
            }
          }
        }
      }
    }
    if (max_signal == 0){
      if ((b-current_b) > 2*pr and change){
        ++current_threat;
        new_threat = true;
      }
      else
        finish = true;
    }
    else{
      place_router(next_router);
      previous_r = next_router;
      paths[current_threat].push_back(next_path);
      current_b += next_path.cost;
    }

  }
  cerr << current_threat << endl;
}

int n,m;
bool valid_output(string file){
  int previous_x=-1, previous_y=-1, x, y;
  ifstream ifs (file);
  bool valid;
  bool local_connection[N_MAX][N_MAX];

  for (int i=0,j=0; i<h and j<w; ++i,++j)
    local_connection[i][j] = false;

  ifs >> n;
  for (int i=0; i<n; ++i){
    valid = false;
    ifs >> x >> y;
    if (x != backbone.x or y != backbone.y){
      if (abs(x-backbone.x)<=1 and abs(y-backbone.y)<=1){
        local_connection[x][y] = true;
        valid = true;
      }
      else if (abs(x-previous_x)<=1 and abs(y-previous_y)<=1){
        local_connection[x][y] = true;
        valid = true;
      }
    }

    if (!valid){
      cerr << "Error in output, first condition"<< endl;
      cerr << previous_x << " " << previous_y << ".." << x <<" "<<y<<"-->" <<backbone.x<<" "<<backbone.y<<endl;
      return(false);
    }

    previous_x = x;
    previous_y = y;
  }

  ifs >> m;
  for (int i=0; i<m; ++i){
    ifs >> x >> y;
    if (local_connection[x][y] == false or building[x][y] == wall){
      cerr << "Error in output, second condition"<< endl;
      return(false);
    }
  }
  ifs.close();

  if (n*pb+m*pr > b){
    cerr << "Error in output, third condition"<< endl;
    return(false);
  }

  return(true);
}
int score(string file){
  int score = 0;
  if (valid_output(file)){
    score = b-current_b;

    for (int i=0; i<h; i++){
      for(int j=0; j<w; j++){
        if (signal[i][j])
          score += 1000;
      }
    }
  }

  return(score);
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
  for (int i=0; i<NPATHS; ++i){
    if (paths[i].size() > 0){
      for (auto s : paths[i])
        total += s.points.size();
    }
  }

  cout << total << endl;
  for (int i=0; i<NPATHS; ++i){
    if (paths[i].size() > 0){
      for (auto p : paths[i]){
        for (auto s : p.points)
          cout << s.x << " " << s.y << endl;
      }
    }
  }

  // print Routers
  cout << p_routers.size() << endl;
  for (auto p : p_routers)
    cout << p.x << " " << p.y << endl;

}


signed main(int argc, char *argv[]) {
  // Files: https://hashcode.withgoogle.com/past_editions.html

  bool debugging = false;
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

  int t=0;
  for (int i=0; i<h; ++i){
    for (int j=0; j<w; ++j)
      if (building[i][j] == target)
        t += 1000;
  }
  cerr << t << "/" << b <<endl;
  cerr << score(file_out)<< "/"<< current_b << endl;
  if (debugging)
    if (valid_output(file_out))
      cerr << "hiphip" <<endl;

  return 0;
}
