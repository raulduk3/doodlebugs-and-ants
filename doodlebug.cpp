/**
 * @author Richard Alvarez
 * @created 2024-01-07
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <ctime>
#include <iterator>
using namespace std;

// Simulation constants
static const int INIT_ANTS = 100;
static const int INIT_DOODLES = 5;
static const int ANT_BREED = 3;
static const int DOODLE_BREED = 8;
static const int DOODLE_STARVE = 3;

class World;
class Organism;
class Ant;
class Doodlebug;

typedef vector<pair<int, int>> Directions;
typedef vector<vector<Organism *>> OrgGrid;

/**
 * Base class: Organism
 */
class Organism
{
protected:
    int x, y;
    int breedCount;
    char character;

public:
    Organism(int x, int y, char ch = ' ')
        : x(x), y(y), breedCount(0), character(ch) {}

    virtual ~Organism() {}

    virtual bool starve() { return false; }

    void incBreed() { breedCount++; }
    void resetBreed() { breedCount = 0; }
    int getBreedCount() { return breedCount; }

    int getX() const { return x; }
    int getY() const { return y; }
    void setPos(int nx, int ny)
    {
        x = nx;
        y = ny;
    }

    virtual void update(World &w) {}
    virtual void print(ostream &os) const { os << character; }

    friend ostream &operator<<(ostream &os, const Organism *organism)
    {
        organism->print(os);
        return os;
    }
};

/**
 * Derived class: Ant
 */
class Ant : public Organism
{
public:
    Ant(int x, int y)
        : Organism(x, y, 'o') {}

    void update(World &w) override;
};

/**
 * Derived class: Doodlebug
 */
class Doodlebug : public Organism
{
private:
    int starveCount;

public:
    Doodlebug(int x, int y)
        : Organism(x, y, 'X'), starveCount(0) {}

    void update(World &w) override;
    bool starve() override
    {
        return (starveCount >= DOODLE_STARVE);
    }
};

/**
 * World class
 */
class World
{
private:
    int size, age;
    OrgGrid grid;
    vector<Organism *> allOrgs;

public:
    World(int size)
        : size(size), age(0),
          grid(size, vector<Organism *>(size, nullptr))
    {
        srand(static_cast<unsigned>(time(nullptr)));
    }

    ~World()
    {
        for (int r = 0; r < size; r++)
        {
            for (int c = 0; c < size; c++)
            {
                delete grid[r][c];
            }
        }
    }

    bool inBounds(int x, int y) const
    {
        return (x >= 0 && x < size && y >= 0 && y < size);
    }

    Organism *getCell(int x, int y) const
    {
        if (!inBounds(x, y))
            return nullptr;
        return grid[x][y];
    }

    void setCell(int x, int y, Organism *org)
    {
        if (!inBounds(x, y))
            return;
        grid[x][y] = org;
    }

    /**
     * Removes occupant from the grid and from allOrgs
     */
    void deleteCell(int x, int y)
    {
        if (!inBounds(x, y))
            return;
        Organism *toDelete = grid[x][y];
        if (toDelete)
        {
            // Remove from our master list
            auto it = find(allOrgs.begin(), allOrgs.end(), toDelete);
            if (it != allOrgs.end())
            {
                allOrgs.erase(it);
            }
            delete toDelete;
            grid[x][y] = nullptr;
        }
    }

    Directions getNeighbors(int x, int y) const
    {
        static int dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
        Directions result;
        for (auto &d : dirs)
        {
            int nx = x + d[0];
            int ny = y + d[1];
            if (inBounds(nx, ny))
            {
                result.push_back({nx, ny});
            }
        }
        return result;
    }

    // Create an Ant and track it in allOrgs
    void createAnt(int x, int y)
    {
        if (!getCell(x, y))
        {
            Ant *a = new Ant(x, y);
            setCell(x, y, a);
            allOrgs.push_back(a);
        }
    }

    // Create a Doodlebug and track it in allOrgs
    void createDoodlebug(int x, int y)
    {
        if (!getCell(x, y))
        {
            Doodlebug *d = new Doodlebug(x, y);
            setCell(x, y, d);
            allOrgs.push_back(d);
        }
    }

    /**
     * Places the initial set of ants & doodles
     */
    void initialize()
    {
        int placedAnts = 0;
        int placedDoodles = 0;

        while (placedDoodles < INIT_DOODLES)
        {
            int x = rand() % size;
            int y = rand() % size;
            if (!getCell(x, y))
            {
                Doodlebug *d = new Doodlebug(x, y);
                setCell(x, y, d);
                allOrgs.push_back(d);
                placedDoodles++;
            }
        }

        while (placedAnts < INIT_ANTS)
        {
            int x = rand() % size;
            int y = rand() % size;
            if (!getCell(x, y))
            {
                Ant *a = new Ant(x, y);
                setCell(x, y, a);
                allOrgs.push_back(a);
                placedAnts++;
            }
        }
    }

    /**
     * We take a snapshot of allOrgs to avoid issues
     * if an organism gets deleted/bred during iteration.
     */
    void update()
    {
        age++;
        // Build a snapshot
        vector<Organism *> snapshot = allOrgs;

        // Shuffle snapshot to randomize update order
        static random_device rd;
        static mt19937 gen(rd());
        shuffle(snapshot.begin(), snapshot.end(), gen);

        // For each occupant in the snapshot
        for (auto o : snapshot)
        {
            // If it was removed in the middle (starved or eaten), skip
            if (find(allOrgs.begin(), allOrgs.end(), o) == allOrgs.end())
                continue;

            o->update(*this);
        }
    }

    friend ostream &operator<<(ostream &os, const World &w)
    {
        os << "World at iteration " << (w.age + 1) << ":\n";
        for (int r = 0; r < w.size; r++)
        {
            for (int c = 0; c < w.size; c++)
            {
                if (w.grid[r][c])
                    os << w.grid[r][c] << ' ';
                else
                    os << "- ";
            }
            os << "\n";
        }
        return os;
    }
};

/**
 * Ant::update(World&)
 * 1) Move (random)
 * 2) Breed if breedCount >= ANT_BREED
 */
void Ant::update(World &w)
{
    // (1) Attempt to move
    Directions neighbors = w.getNeighbors(getX(), getY());
    static random_device rd;
    static mt19937 gen(rd());
    shuffle(neighbors.begin(), neighbors.end(), gen);

    for (auto &nbr : neighbors)
    {
        int nx = nbr.first;
        int ny = nbr.second;
        if (!w.getCell(nx, ny))
        {
            w.setCell(nx, ny, this);
            w.setCell(getX(), getY(), nullptr);
            setPos(nx, ny);
            break;
        }
    }

    // (2) Breed
    incBreed();
    if (getBreedCount() >= ANT_BREED)
    {
        shuffle(neighbors.begin(), neighbors.end(), gen);
        for (auto &nbr : neighbors)
        {
            int nx = nbr.first;
            int ny = nbr.second;
            if (!w.getCell(nx, ny))
            {
                w.createAnt(nx, ny);
                break;
            }
        }
        resetBreed();
    }
}

/**
 * Doodlebug::update(World&)
 * 1) Starve check
 * 2) Attempt to eat
 * 3) If no eat, move
 * 4) Breed if breedCount >= DOODLE_BREED
 */
void Doodlebug::update(World &w)
{
    // 1) Starve check
    if (starve())
    {
        w.deleteCell(getX(), getY());
        return;
    }

    bool ate = false;
    Directions neighbors = w.getNeighbors(getX(), getY());
    static random_device rd;
    static mt19937 gen(rd());
    shuffle(neighbors.begin(), neighbors.end(), gen);

    // 2) Attempt to eat an adjacent Ant
    for (auto &nbr : neighbors)
    {
        int nx = nbr.first;
        int ny = nbr.second;
        Organism *occupant = w.getCell(nx, ny);
        // Is occupant an Ant?
        Ant *maybeAnt = dynamic_cast<Ant *>(occupant);
        if (maybeAnt)
        {
            // Eat (remove occupant) and move
            w.deleteCell(nx, ny);
            w.setCell(nx, ny, this);
            w.setCell(getX(), getY(), nullptr);
            setPos(nx, ny);
            ate = true;
            starveCount = 0;
            break;
        }
    }

    // 3) If didn't eat, try to move
    if (!ate)
    {
        shuffle(neighbors.begin(), neighbors.end(), gen);
        bool moved = false;
        for (auto &nbr : neighbors)
        {
            int nx = nbr.first;
            int ny = nbr.second;
            if (!w.getCell(nx, ny))
            {
                w.setCell(nx, ny, this);
                w.setCell(getX(), getY(), nullptr);
                setPos(nx, ny);
                moved = true;
                break;
            }
        }
        // If we didn't eat, increment starveCount
        // even if we moved. The doodlebug starves if it doesn't eat
        // for DOODLE_STARVE turns.
        starveCount++;
    }

    // 4) Breed
    incBreed();
    if (getBreedCount() >= DOODLE_BREED)
    {
        shuffle(neighbors.begin(), neighbors.end(), gen);
        for (auto &nbr : neighbors)
        {
            int nx = nbr.first;
            int ny = nbr.second;
            if (!w.getCell(nx, ny))
            {
                w.createDoodlebug(nx, ny);
                break;
            }
        }
        resetBreed();
    }
}

/**
 * main
 */
int main()
{
    World w(20);
    w.initialize();

    while (true)
    {
        cout << w << endl;
        cout << "Press Enter to continue, or type 'q' (then Enter) to quit.\n";
        string input;
        if (!std::getline(cin, input))
        {
            break;
        }
        if (input == "q")
        {
            break;
        }
        w.update();
    }

    return 0;
}
