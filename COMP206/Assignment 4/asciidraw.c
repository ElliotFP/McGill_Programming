#include <stdio.h>
#include <string.h>
#include <math.h>

// Author: Elliot Forcier-Poirier
// Faculty: Mathematics and Statistics
// Date: 11/11/2022

int main()
{
	// initialize your variables.
	char dchar = '*';
	char function[10];
	int i,j, par1, par2;
	
	//While loop that only accepts END, CHAR, GRID.
	do
	{
		scanf("%s", function);
		// If function is GRID break the loop and initialize the grid.
		if (strcmp("GRID", function) == 0)
		{
			break;
		}

		// Check that command was CHAR, otherwise say that that the grid must be set first.
		else if (strcmp("CHAR", function) == 0)
		{
			scanf("%1s", &dchar);
			fflush(stdin);
		}
		
		// End the program when we write END
		else if (strcmp("END",function) == 0)
                {                	
			break;
                }

		//Return that we can't accept any other functions than CHAR until we have a grid.
		else
		{
			puts("Only CHAR command is allowed before the grid size is set. \n");
			fflush(stdin);
		}
	}
	while (1);
	
	//take the parameters of the grid.
	scanf("%d %d", &par1, &par2);

	// Create the grid, I added plus one since I'm factoring in the axis.
	char grid[par2 + 1][par1 + 1];
	
        // Initialize the empty grid
        for (i = 0; i < par2 + 1; i++)
        {
                for (j = 0 ; j < par1+1 ; j++)
                {
                        grid[i][j] = ' ';
                }
	} 

	//Loop until we get the END command.
	do
	{
		scanf("%s", function);
		
		//Exit if the command is END.
                if (strcmp("END",function) == 0)
                {
                        break;
                }

		//DISPLAY function prints the grid
		else if (strcmp("DISPLAY", function) == 0)
		{
			// Create the borders and the number demarkations for the x axis.
       			for (i = 0; i < par1 + 1; i++)
        		{
                		char index = i%10 + '0';
                		grid[par2][i+1] = index;
        		}

        		// Create the borders and the number demarkations for the y axis.
        		for (i = 0; i < par2 + 1; i++)
        		{
                	char index = i%10 + '0';
			grid[par2 - i - 1][0] = index;
        		}
			
			//Print out the array, one character at a time
			for (i = 0; i < par2 + 1 ; i++)
        		{	 
               			for (j = 0; j < par1 + 1 ; j++)
               			{
                       			putchar(grid[i][j]);
               			}
               			putchar('\n');
       			}
			fflush(stdin);
		}
		
		//CHAR function to change the drawing character.
		else if (strcmp("CHAR", function) == 0)
                {
                        //scanf("%1s", dchar);
                        fflush(stdin);
                }

		// Drawing a rectangle
		else if (strcmp("RECTANGLE", function) == 0)
		{
			int x_1, y_1, x_2, y_2;
			scanf("%d,%d %d,%d", &x_1, &y_1, &x_2, &y_2);
			
			//format with the grid
			x_1++;
			x_2++;
			y_1 = par2 - y_1 - 1;
			y_2 = par2 - y_2 - 1;

			//Drawing horizontal lines.
			for (i = x_1 ; i <= x_2 ; i++)
			{
				grid[y_1][i] = dchar;
				grid[y_2][i] = dchar;
			}

			//Drawing vertical lines.
                        for (i = y_1 ; i <= y_2 ; i++)
                        {
                                grid[i][x_1] = dchar;
                                grid[i][x_2] = dchar;
                        }
		}

		// Drawing a circle
		else if (strcmp("CIRCLE", function) == 0)
		{
			int x, y, r;
                        float yfloat;
			scanf("%d,%d %d", &x, &y, &r);
			
			//format it with the grid
			x++;
			y = par2 - y - 1;	
			
			for( i = 0 ; i < r + 1 ; i++ )
    			{
				//We find the points for each quadrant of the circle and using symmetry along the axis
				float yfloat = sqrt(pow(r,2) - pow(i,2));
				int yround = (int)ceil(yfloat);

				//Basically you overlap all possible half circles to assure that you don't miss a character in the circle
				grid[y + yround ][x + i] = dchar;
				grid[y + yround ][ x - i] = dchar;
				grid[y - yround ][x + i] = dchar;
                                grid[y - yround ][x - i] = dchar;
				grid[y + i ][ x - yround] = dchar;
                                grid[y + i ][x + yround] = dchar;
				grid[y - i ][ x - yround] = dchar;
                                grid[y - i ][x + yround] = dchar;
			}
		}

		//CHAR function
		else if (strcmp("CHAR", function) == 0)
                {
                        scanf("%1s", &dchar);
                        fflush(stdin);
                }

		//Drawing a line
		else if (strcmp("LINE", function) == 0)
		{
			int x_1, y_1, x_2, y_2;
                        scanf("%d,%d %d,%d", &x_1, &y_1, &x_2, &y_2);
			
			//format it with the grid
                        x_1++;
                        x_2++;
                        y_1 = par2 - y_1 - 1;
                        y_2 = par2 - y_2 - 1;
			
			// Vertical Line
			if (x_1 == x_2)
			{
				if (y_1 < y_2)
                                {
	     				for(i = y_2 ; i <= y_1 ; i++)
        	                        {
                                        	grid[i][x_1] = dchar;
	                                }

                                }
                                else
                                {
					for(i = y_1 ; i <= y_2 ; i++)
					{
				 		grid[i][x_1] = dchar;
					}
				}
			}
		
			// Horizontal Line	
			else if (y_1 == y_2)
                        {
                                if (x_1 < x_2)
                                {                               
                                        for (i = x_1 + 1 ; i <= x_2 + 1 ; i++)
                                        {
                                                grid[y_1][i] = dchar;
                                        }

                                }
                                else
                                {
                                       for (i = x_2 + 1 ; i <= x_1 - x_2 + 1 ; i++)
                                        {
                                                grid[y_1][i] = dchar;
					}
                                }       
                        }

			else
			{
				//initialize the beginning and end of the line.
				int x_f, y_f, flr;
				float slope, y, x;
				(x_1 < x_2) ?  ( x = x_1, x_f = x_2, y = (float)y_1, y_f = y_2) : ( x = x_2, x_f = x_1, y = (float)y_2, y_f = y_1 ) ; 
				slope = (float) (y_f - y)/(x_f - x);				
	
				//When the line is more horizontal than vertical.
				if ((slope < 1) && ( slope > -1))
				{	
					flr = y;	

					for ( i = (int) x ; i <= x_f ; i++ )
					{
						grid[flr][i] = dchar;
						y -= slope;
						flr = floor(y);
						grid[flr][i] = dchar;
					}	
				}

				//diagonal line.
				else if ( ( slope - 1 == 0) || (slope + 1 == 0) )
				{	
					for (i = x ; i <= x_f ; i++)
                                        {
					grid[(int) y][i] = dchar;
					y -= slope;
					}
				}

				//When the line is more vertical than horizontal
				else 
				{
					flr = x ;
					
					//take the reciprocal of the slope to facilitate the process.
					slope = pow(slope, -1);

					for (i = 0; i <= ((y < y_f) ? (y_f - y) : (y - y_f)); i++)
					{
						if (slope > 0)
						{
							grid[(int)y - i][flr] = dchar ;
							x += slope;
							flr = floor(x);
							grid[(int)y - i][flr] = dchar ;
						}
						else
						{
							grid[(int)y + i][flr] = dchar ;
							x -= slope;
							flr = floor(x) ;
							grid[(int)y + i][flr] = dchar ;
						}
						
					}
				}
			}
		}

		else if (strcmp("GRID", function) == 0)
		{
			printf("Cannot change grid after initializing \n");
		}

		//If anything else is written in the console, return an error.
		else
		{
			printf("Error did not understand %s \n", function);
		}

	}
	while (1);
}

