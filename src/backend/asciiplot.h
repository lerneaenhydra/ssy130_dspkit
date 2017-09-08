/** @file A simple ASCII-plot function  */

#ifndef ASCIIPLOT_H_
#define ASCIIPLOT_H_ 

/* Useage example;

	#include <stdout.h>
	#include "asciiplot.h"

	int main(void){
		int len = 101;
		float y[len];
		float x[len];
		int i;
		for(i = 0; i < len; i++){
			y[i] = i*i/((float) len * len);
			x[i] = ((float)i)/len;
		}
		//Plotting (x,y) will now draw x^2 for x = [0,1]
		
		float axis[4] = {NAN, NAN, NAN, NAN};
		struct asciiplot_s plot = {
			.cols = len,
			.rows = 40,
			.xdata = x,
			.ydata = y,
			.data_len = len,
			.xlabel = "x",
			.ylabel = "y",
			.title = "title",
			.axis = axis,
			.label_prec = 4,
		};
		asciiplot_draw(&plot);
		
		return 0;
	}
	
śGenerates;
                                                 title
 .9803+----------------------------------------------------------------------------------------------+
      |                                                                                              *
      |                                                                                            **|
      |                                                                                           *  |
      |                                                                                         **   |
      |                                                                                        *     |
      |                                                                                       *      |
      |                                                                                     **       |
      |                                                                                    *         |
      |                                                                                   *          |
      |                                                                                  *           |
      |                                                                                **            |
      |                                                                              **              |
      |                                                                             *                |
      |                                                                           **                 |
      |                                                                         **                   |
      |                                                                        *                     |
      |                                                                      **                      |
      |                                                                    **                        |
      |                                                                  **                          |
     y|                                                                **                            |
      |                                                              **                              |
      |                                                            **                                |
      |                                                           *                                  |
      |                                                         **                                   |
      |                                                       **                                     |
      |                                                     **                                       |
      |                                                  ***                                         |
      |                                               ***                                            |
      |                                             **                                               |
      |                                          ***                                                 |
      |                                       ***                                                    |
      |                                    ***                                                       |
      |                                 ***                                                          |
      |                             ****                                                             |
      |                        *****                                                                 |
      |                  ******                                                                      |
      |           *******                                                                            |
     0************-----------------------------------------------------------------------------------+
      0                                              x                                           .9901
 */ 

struct asciiplot_s {
	int cols;				//!<- Set to the number of columns to print (length of x-axis)
	int rows;				//!<- Set to the number of rows to print (length of y-axis)
	float * const xdata;	//!<- Set to pointer to x-data. Set to NULL to imply xdata = {0, 1, 2, 3, ...} of length data_len.
	float * const ydata;	//!<- Set to pointer to y-data
	int data_len;			//!<- Set to number of elements in x/y-data
	char * const xlabel;	//!<- Set to string to print as x-axis label (or NULL to disable)
	char * const ylabel;	//!<- Set to string to print as y-axis label (or NULL to disable)
	char * const title;		//!<- Set to string to print as title (or NULL to disable)
	float * axis;			//!<- Point to array to control axis extents [xmin, xmax, ymin, ymax]. Set any element to NaN to enable auto-scaling for the relevant element(s). MUST BE AN ARRAY OF LENGTH 4!
	int label_prec;			//!<- Set to the number of decimals to include for the axis extent labels. 2-5 are typical values.
};

/** @brief Draws a plot to stdout */
void asciiplot_draw(struct asciiplot_s * const plot);


#endif /* ASCIIPLOT_H_ */
