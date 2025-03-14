# utils - Utilities for working with stemma outputs and inputs
In this repository:
* **stats**: Compute various statistics for set of collated manuscripts.
* **chomp**: trim output from stemma runs

# stats: Textual Variation Analysis Tool
## Description
`stats` is a command-line tool designed for analyzing textual variations among different witnesses (manuscripts) at various sites (positions in the text). It reads a file containing the witnesses and their readings, then provides tools to calculate similarities, distances, and statistical measures to understand the relationships between the witnesses. This program is particularly useful for researchers in textual criticism or phenetics, offering insights into manuscript relationships through metrics like similarity rates, medoid distances, and the Matthews Correlation Coefficient (MCC).
## Usage
`./stats <mss>.tx [opt] <ms-name>`
* `<mss>.tx`: The input file containing the witnesses and their readings.
* `[opt]`: Optional flag, either `-med` or `-mc`.
* `<ms-name>`: The name of the manuscript to analyze, or the option flag.
## Input File Format
The input file must follow this format:
* **First line**: Two integers, `nLeafs` and `nSite`s, representing the number of witnesses and the number of sites, respectively.
* **Subsequent lines**: Each line contains the name of a witness followed by its readings at each site, separated by a space.
### Example:
```
5 10
Wit1 0123012301
Wit2 0123012301
Wit3 0123012301
Wit4 0123012301
Wit5 0123012301
```
## Options
* **No option**: If a manuscript name is provided, the program calculates and prints similarity statistics with all other manuscripts. This includes rates and counts for different types of agreements (Overall, Type-A, Type-AB, Type-B) and the Relationship Number (RN).
* `-med`: Calculates and prints the sum of distances for each witness, useful for identifying medoids (central witnesses in terms of distance).
* `-mcc`: Calculates and prints the Matthews Correlation Coefficient (MCC) for binary classification at each site and variant, based on a set of witnesses specified after the option.
## Environment Variables
* `A=`: If set, specifies the index of the archetype witness (overrides default).
* `B=`: If set, specifies the index of the Byzantine witness (overrides default).
## Key Concepts
* **Witnesses**: Different manuscripts or versions of the text.
* **Sites**: Positions in the text where variations occur.
* **Readings**: The specific variant at a site for a witness (e.g., `0`, `1`, `?`).
* **Similarity**: Measured by the rate of agreement between two witnesses across all sites.
* **Distance**: Calculated as `1.0 - similarity`.
* **Medoids**: Witnesses with the smallest sum of distances to all other witnesses, indicating central positions in the dataset.
* **Matthews Correlation Coefficient (MCC)**: A measure of binary classification quality, assessing how well a variant at a site distinguishes a group of witnesses.
* **Waltz Relationship Number (RN)**: A measure of the relationship between two witnesses, based on the frequency of their agreements relative to the overall frequency of readings.
## Building
The program is written in C and can be compiled using a standard C compiler, such as GCC:
```
gcc -o stats stats.c
```
Ensure that any required header files, such as scc.h, are in the include path. This header likely contains utility macros (e.g., ASSERT, NEWMEM) and should be included in the repository.
## Examples
Given a file mss.tx with the following content:
```
3 5
WitA 01230
WitB 01231
WitC 01232
```
### Analyze similarities for WitA:
```
./stats mss.tx WitA
```
Output might look like:
```
WitA    WitA     |O: 1.000 5 5  |A: 0.000 0 0  |AB: 0.000 0 0  |B: 0.000 0 0  || 1
WitA    WitB     |O: 0.800 4 5  |A: 0.000 0 1  |AB: 0.000 0 1  |B: 0.000 0 1  || 0.8
WitA    WitC     |O: 0.600 3 5  |A: 0.000 0 2  |AB: 0.000 0 2  |B: 0.000 0 2  || 0.6
```
(Note: piping this out to `| sort +4` will sort the lines based on the percentage Type-O agreements.)
### Calculate distances for medoids:
```
./stats mss.tx -med 
```
Output might look like:
```
WitA    0.4000
WitB    0.3333
WitC    0.4667
```
### Calculate MCC for witnesses WitA and WitB:
```
./stats mss.tx -mcc WitA WitB
```
Output might look like:
```
0 0 phi:  0.5000  WitA WitB [! !]
1 1 phi:  0.5000  WitA WitB [! !]
2 2 phi:  0.0000  WitA WitB [!WitC !]
```
### Print agreements between two witnesses 
* `./stats mss.tx -O witA witB` for overall agreements
* `./stats mss.tx -A witA witB` for non-Archetypal agreements
* `./stats mss.tx -B witA witB` for non-Byzantine (majority) agreements
* `./stats mss.tx -AB witA witB` for non-Archetypal, non-Byzantine agreements.
These options assume that a file `SOLN.VARS` is present in the directory or redirected in.
## Notes
* **Readings**: Valid characters are from the set `?0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-`.
* **Lacunae**: Missing readings are represented by ? and are excluded from calculations.
* **Archetype and Byzantine Witnesses**: Default to calculated values unless overridden by environment variables A and B.
## License
This project is licensed under the MIT License.

# Chomp
Chomp is a C program that reads from standard input, processes the input by handling carriage return (`\r`) characters to overwrite the current line buffer, and prints the final state of the line when a newline (`\n`) is encountered. This behavior makes it particularly useful for cleaning up output from programs that use `\r` to update the same line, such as progress indicators, while ensuring consistent line printing for inputs with Unix-style (`\n`) or Windows-style (`\r\n`) line endings.
## Usage
To compile the program, use a C compiler like `gcc`:
```
bash
gcc -o chomp chomp.c
```
To run the program, pipe input to it or redirect from a file:
```
bash
./chomp < input.txt
```
or
```
bash
cat input.txt | ./chomp
```
## Example
Consider a file `input.txt` with the following content (where `\r` represents a carriage return and `\n` a newline):
```
Progress: 10%\rProgress: 20%\rProgress: 100%\nHello\r\nWorld\n
```
Running `./chomp < input.txt` will produce the following output:
```
Progress: 100%
Hello
World
```
## Explanation
* For `Progress: 10%\rProgress: 20%\rProgress: 100%\n`:
** Each `\r` overwrites the buffer with the subsequent text.
** When `\n` is encountered, it prints the final state, `Progress: 100%`.
* For `Hello\r\n`, the `\r` prepares the buffer, and `\n` prints `Hello`.
* For `World\n`, the `\n` triggers printing of World.
## How It Works
The program:
* Uses a static 256-character buffer (`linebuf`) to store input characters.
* Reads characters from standard input using `getchar()` until `EOF` is encountered.
* Processes each character as follows:
  * **Newline (`\n`)**: Prints the current buffer content using `puts()` (which adds a newline) and then resets the buffer.
  * **Carriage return (`\r`)**: Null-terminates the buffer and resets the pointer to the start, effectively allowing subsequent characters to overwrite the buffer.
  * **Other characters**: Adds them to the buffer, advancing the pointer.
Due to the lack of a `break` statement after the `\n` case in the switch, when `\n` is encountered, it executes both the print operation and the buffer reset (via the `\r` case). For `\r\n` sequences, the `\r` resets the buffer, and the `\n` prints the prior content, effectively ignoring the `\r` in the output.
## Limitations
* **Buffer Size**: Assumes lines are shorter than 256 characters. Longer lines may cause buffer overflow and undefined behavior.
* **Output Trigger**: Only prints when \n is encountered. Lines terminated solely by `\r` or partial lines without a trailing `\n` (e.g., at `EOF`) will not be printed.
* **Specific Use Case**: Optimized for `\n` or `\r\n` line endings; it does not fully support old Mac-style `\r`-only line endings.
This program is a simple yet effective tool for processing text streams where carriage returns need to be handled or removed, ensuring output is printed cleanly on newlines.
