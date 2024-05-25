# :bulb: Overlap Wave Function Collapse

This is a C++ implementation of Overlap Wave Function Collapse.

## :computer: How it works

This implementation is split into three parts. Analyzer, Backtracker and the WFC.

First the Analyzer reads the input image from which it extracts patterns with their frequencies and rules.

<details>
<summary> Learn more about patterns and rules </summary>
<h3>Patterns</h3>
are the sub-images of the input image. They are extracted by sliding a window of size `pattern_size` over
the input image and storing the sub-image in a list if it is not already present. This is done by converting the
patterns pixels to a string and storing it in a hashmap as key. This could be done more efficiently by using a hash
function that takes the image as input and returns a hash.

For each unique pattern is also saved the total number of times it appears in the input image.
This is then used to calculate the probability of each pattern occurring in the output image in order to generate output 
images that are more similar to the input image.

<h3>Rules</h3>
determine which patterns can be placed next to each other at a certain offset.
Lets imagine that we have two patterns where 1 represents white and 0 black:

```
p1:    p2:
1 1 1 | 1 1 1
1 0 1 | 1 0 1
1 0 1 | 1 0 0
```

Now if we were to check if they can be placed next to each other at offset (1, 0) we would compare these two crops:

crops:
```
p1     p2:
1 1 1 | 1 1 1
1 0 1 | 1 0 1
```

As you can see they are the same so they can be placed next to each other at offset (1, 0).

But if we were to check if they can be placed next to each other at offset (1, 0) we would compare these two crops
and notice that they are not the same and thus they cannot be placed next to each other at offset (1, 0).

crops:
```
p1:  p2:
1 1 | 1 1
0 1 | 0 1
0 1 | 0 0
```
</details>

After the Analyzer has extracted the patterns and rules the WFC can start generating. If the Backtracker is enabled the
algorithm will try to backtrack if it arrives at contradiction.

<details>
<summary>Lear more about how algorithm generates the image</summary>


- Before the WFC starts generating the output image, it first creates a grid that represents the current state of the
image. Each cell in the grid is a set of patterns that can be placed at that position. They are in
what's called a superposition.


- The first step in the Wave Function Collapse algorithm is known as Observation. During this
step, the algorithm finds the cell with the lowest entropy and collapses it. This means that it calculates
the shannon entropy for each not collapsed cell and collapses the one with the lowest entropy.


- The second step is called propagation. During this step, the algorithm propagates the information about
the collapsed cell to its neighbors. This is done by removing the patterns that are not compatible
with the collapsed cell. As a result, the neighbors will have some patterns removed, and they will propagate
this information to their neighbors. This step is also sometimes called as wave because of its ripple effect, where
changes propagate outwards from the point of origin. The propagation step is repeated until no more patterns
can be removed.


- Then the algorithm goes into observation again and continues this cycle. It's also possible that the algorithm
will arrive at contradiction. This happens when any cell in a grid will have no possible patterns left it can 
collapse into. When this happens contradiction is found and the algorithm will either backtrack or stop.
</details>

<details>
<summary>Learn about backtracker</summary>

In simple terms, the backtracker is a way to go to previous state of the algorithm whenever it finds a contradiction
and to try a different solution.

In each step when the algorithm collapses a cell, it will also save the state into backtracker if enabled.
Then In the observation step if the algorithm arrives at contradiction it will start backtracking instead.

The backtracker has two important factors. The first one is the depth. This is the number of steps the algorithm can
go back in the history. The second one is the max iterations. This tells the backtracker how many times it should try
a single state before it gives up and goes back further in the history.

If the algorithm is backtracking, it will store the states in a different dequeue. If it finds a possible solution it
will merge it to the main dequeue and continue to generate the image. It knows when to stop backtracking by comparing 
the current iteration to the last iteration when it found a contradiction.

</details>

## :hammer: How to install

### Linux

Because this project uses cimg you need to download dependencies first via apt or any other package manager you use.

```shell
apt-get install libx11-dev
apt-get install libjpeg-dev
apt-get install libpng-dev
```

Then you can clone the repository and create build directory

```shell
git clone https://github.com/Luniminex/Overlap-WFC.git
cd Overlap-WFC
mkdir build
cd build
```

Next, you need to compile the project. You can do this by running the following commands:

```shell
cmake ..
make
```

Now you have two executables in the build directory. The first one is WFC which does not have any optimazations and
the second one is WFC_optimized which uses the -O3 flag for optimization.

```shell
./WFC
cd ../outputs/solutions/
explorer.exe solution.png //shows the output image, or use ls to see if it was created
```

If you see the output image, it should work as intended.

When you run either of them without any arguments

## :rocket: How to use

This repository contains a simple example of how to use the Overlap Wave Function Collapse algorithm.
It uses cxxopts https://github.com/jarro2783/cxxopts for parsing command line arguments.

An example usage that enables rotation, flipping, backtracking, saving iterations to a file
and setting the output image size to 32x32 would look like this:
```shell
./wfc -r -f -e -y -w 32 -h 32
```

You can also use the help command to see all available options:
```shell
./wfc -h
```

### Windows and other platforms

I have only tested this on Linux. If you use windows I strongly recommend using WSL for this.

### :memo: Notes

please note that this is my understanding of the WFC and it could be wrong. If you find any mistakes or have any 
suggestions feel free to correct me or suggest changes.

If you want to learn more about Wave Function Collapse I suggest you read this paper, which was a huge help.
- https://adamsmith.as/papers/wfc_is_constraint_solving_in_the_wild.pdf

I also recommend reading through this repository that explains much better than I do how the algorithm works.
- https://github.com/mxgmn/WaveFunctionCollapse

This projects also uses the cimg library for reading and writing images.

- https://cimg.eu/

And the cxxopts library for parsing command line arguments.

- https://github.com/jarro2783/cxxopts