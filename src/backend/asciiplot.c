#include "asciiplot.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef SSY130_DEVKIT
//Handle special hardware cases manually
#include "printfn/printfn.h"
#include "hw/board.h"
#define snprintf snprintfn
#define putchar(x) board_usart_write(x)
#else
//Assume stdio exists
#include <stdio.h>
#endif

/** @brief Returns the number of elements in an array */
#define NUMEL(x)	(sizeof(x)/sizeof(x[0]))

/** @brief Return true if the configured plot structure at least seems reasonable */
bool asciiplot_verify(struct asciiplot_s * const plot){
	if(plot->cols <= 0 || plot->rows <= 0 || plot->ydata == NULL || plot->data_len <= 0 || plot->label_prec < 1){
		return false;
	}else{
		return true;
	}
}

void asciiplot_draw(struct asciiplot_s * const plot){
	if(!asciiplot_verify(plot)){
		printf("ERR: invalid asciiplot setup!\n");
		return;
	}
	
	int i,j;
	//If xdata is empty, seed it with implicit values and point to these values
	float xdata[plot->data_len];
	for(i = 0; i < plot->data_len; i++){
		if(plot->xdata == NULL){
			xdata[i] = i;
		}else{
			xdata[i] = *(plot->xdata + i);
		}
	}
	
	//Determine the range of values present in data
	/* Use negation expression in if statement to force any finite number to
	 * overwrite a NaN axis extent (as any comparison with NaN returns false)
	 */
	float axis[] = {xdata[0], xdata[0], plot->ydata[0], plot->ydata[0]};
	for(i = 1; i < plot->data_len; i++){
		if(!(axis[0] < xdata[i])){
			axis[0] = xdata[i];
		}
		if(!(axis[1] > xdata[i])){
			axis[1] = xdata[i];
		}
		if(!(axis[2] < plot->ydata[i])){
			axis[2] = plot->ydata[i];
		}
		if(!(axis[3] > plot->ydata[i])){
			axis[3] = plot->ydata[i];
		}
	}

	//Handle NaN axis extents, arbitrarily set to zero
	for(i = 0; i < NUMEL(axis); i++){
		if(axis[i] != axis[i]){
			axis [i] = 0;
		}
	}
	
	//Override the auto-ranging axis extents by any manually set range extents
	for(i = 0; i < NUMEL(axis); i++){
		if(*(plot->axis + i) == *(plot->axis + i)){
			axis[i] = *(plot->axis + i);
		}
	}
	
	//Generate strings containing the extents of each axis
	int labellen = 10 + plot->label_prec;
	char axislabel[4][labellen];
	int max_y_labellen = 0;
	for(i = 0; i < NUMEL(axislabel); i++){
		int this_len = snprintf(axislabel[i], labellen, "%.*g", plot->label_prec, axis[i]);
		if(i > 1 && this_len > max_y_labellen){
			max_y_labellen = this_len;		//Get maximum length for last two strings, as these will control how much space we need to reserve for the y axis markers
		}
	}
	
	//Check space needed for y-label
	int ylabellen = strlen(plot->ylabel);
	if(ylabellen > max_y_labellen){
		max_y_labellen = ylabellen;
	}
	
	/* Initialize a "screen buffer" containing all characters to output except the optional title.
	 * Set up;
	 *   - Frame
	 *   - axis ticks and values
	 *   - x/y-labels
	 *   - data points
	 * Do this in this order so any overwrites will keep the last (most important) data type */
	
	//Empty the buffer by placing ' ' characters everywhere
	char screenbuf[plot->cols][plot->rows];
	for(i = 0; i < plot->cols; i++){
		for(j = 0; j < plot->rows; j++){
			screenbuf[i][j] = ' ';
		}
	}
	
	//Draw the frame horzontal and vertical lines
	for(i = max_y_labellen; i < plot->cols; i++){
		screenbuf[i][0] = '-';
		screenbuf[i][plot->rows-2] = '-';
	}
	for(i = 0; i < plot->rows - 1; i++){
		char c;
		if(i == 0 || i == plot->rows - 2){
			c = '+';
		}else{
			c = '|';
		}
		screenbuf[max_y_labellen][i] = c;
		screenbuf[plot->cols-1][i] = c;
	}
	
	//Add axis extents
	//xmin
	for(i = 0; i < strlen(axislabel[0]); i++){
		screenbuf[max_y_labellen + i][plot->rows - 1] = axislabel[0][i];
	}
	
	//xmax
	for(i = 0; i < strlen(axislabel[1]); i++){
		screenbuf[plot->cols - strlen(axislabel[1]) + i][plot->rows - 1] = axislabel[1][i];
	}
	
	//ymin
 	for(i = 0; i < strlen(axislabel[2]); i++){
 		screenbuf[i + max_y_labellen - strlen(axislabel[2])][plot->rows - 2] = axislabel[2][i];
 	}
	
	//ymax
	for(i = 0; i < strlen(axislabel[3]); i++){
		screenbuf[i + max_y_labellen - strlen(axislabel[3])][0] = axislabel[3][i];
	}
	
	//Add axis labels
	for(i = 0; i < strlen(plot->xlabel); i++){
		screenbuf[(plot->cols - max_y_labellen)/2 - strlen(plot->xlabel) + i + max_y_labellen][plot->rows - 1] = plot->xlabel[i];
	}
	
	for(i = 0; i < strlen(plot->ylabel); i++){
		screenbuf[i + max_y_labellen - strlen(plot->ylabel)][plot->rows/2] = plot->ylabel[i];
	}
	
	//Add all data points
	for(i = 0; i < plot->data_len; i++){
		int xpts = plot->cols - max_y_labellen;
		int ypts = plot->rows - 2;
		
		float xv = xdata[i];
		float yv = plot->ydata[i];
		
		float xmin = axis[0];
		float xmax = axis[1];
		float ymin = axis[2];
		float ymax = axis[3];
		
		//If point lies in plot range, find best location and add it to the screen buffer
		if(xv >= xmin && xv <= xmax && yv >= ymin && yv <= ymax){
			float xrel = (xv - xmin) / (xmax - xmin);
			float yrel = (yv - ymin) / (ymax - ymin);
			
			int xp = (xrel * xpts + 0.5);	//Round to nearest integer. As xrel and xpts are known positive this will always be safe.
			int yp = (yrel * ypts + 0.5);
			
			xp += max_y_labellen;		//Offset x by label length
			yp = plot->rows - 2 - yp;	//Y is internally stored with 0 uppermost
		
			//Manually ensure all locations are valid, ignore any that would lie outside the plot range
			if(xp >= 0 && yp >= 0 && xp <= plot->cols - 1 && yp <= plot->rows - 1){
				screenbuf[xp][yp] = '*';
			}
		}
	}
	
	
	//Finally, print the plot to the output, starting with the title if needed
	if(plot->title != NULL){
		int padding = plot->cols/2 - strlen(plot->title)/2;
		for(i = 0; i < padding; i++){
			putchar(' ');
		}
		printf("%s\n", plot->title);
	}
	for(i = 0; i < plot->rows; i++){
		for(j = 0; j < plot->cols; j++){
			putchar(screenbuf[j][i]);
		}
		putchar('\n');
		putchar('\r');
	}
}
