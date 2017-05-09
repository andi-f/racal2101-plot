# racal2101-plot
Racal2101-plot reads the actual value every 2s from the frequencycounter RACAL DANA 2101 and draws a graph.

Functions:
- The values are stored in a csv file. These files can be reopened.

- You can create a png-graphic from the graph.

- The min/max values of axis can be set manualy or automatic.

- The GPBIP address must be changed in the source code.

Compile and install:

The program is written in C and uses GTK-2. 
Just compile it with make.

Additional software:
You need linux-gpib and a gpib interface.

The gpib-functions.h and gpib-functions.h were written by Frank Mori Hess.

