## GENERAL INFO 
The main purpose of the program is to convert a PGM image file into a sketch file or vice versa.
## RUN 
To run this program: 
#### ./converter filename.sk 
or 
#### ./converter filename.pgm
where: filename is the name of the file you want to convert.

## ABOUT THE PROGRAM 
The converter is essentially a matter of 2 functions:

(1) encode() => Takes a PGM image file and converts it into a sketch file.
- Do a run length encoding in one dimension (i.e.: count the number of subsequent occurrences of the current colour) and write to the output file commands that draw the sketch line by line.
		
(2) decode() => Takes a sketch file and converts it into a PGM image file.
- Scan sequences of bytes (commands) that only build data for the colour and for the x position so that it can fill the raster with that colour from the current x position up to the target x position. (i.e.: draw the raster line by line).

Note: I defined height and width as 200 in order to view the sketch file properly when using the sketch viewer. However, the program can convert rasters of any width and length.

	
