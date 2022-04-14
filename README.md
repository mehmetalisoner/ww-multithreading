# ww-multithreading


Whats done so far is when the user types in "./ww -r 10 "somefiledirectory" it will go to that directory and word wrap the text, but if there is a dirctory in the directory
it has a problem where some of the file gets moved to the second sub directory, need to add a way to ignore subdirectories itself from beign wraped



now i was thinking to use a function to replace what i coded, so it could be done recursivley 
--will look at this in a few hours.


Maybe use a queue:
	1	Assign the head of the queue to path.
	2	Remove the head of the queue.
	3	If path references a directory,
	1	Append the path of the files in path to the todo queue.
	4	Perform whatever action you want to perform with path.

