#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Record / Node for the linked list.
// You MUST use the exact structure. No modification allowed.
typedef struct PersonalInfoRecord
{ 
	char id[10];
	char ptype;
	char name[31];

	union
	{
		struct
		{
			char dept[31];
			int hireyear;
			char tenured;
		}prof;
		struct
		{
			char faculty[31];
			int admyear;
		}stud;
	}info;

	struct PersonalInfoRecord *next;
} 
PersonalInfoRecord;

// Function to add an entry to the linked list. 
PersonalInfoRecord* addEntry(char *input, PersonalInfoRecord *pirp)
{
	char *idChar = strsep(&input, ",\n");
	int id = atoi(idChar);
        char *type = strsep(&input, ",\n");
       	char *name = strsep(&input, ",\n");
	char *depFac = strsep(&input, ",\n");
	int year = atoi(strsep(&input, ",\n"));


	//Initialise and set the 3 parameters that are shared between Student and Prof.
 	PersonalInfoRecord *pir = (PersonalInfoRecord*)malloc(sizeof(PersonalInfoRecord)) ;
	strcpy(pir->id, idChar); 
	pir->ptype = type[0];
       	strcpy(pir->name, name);
	pir->next = NULL;

	//Check if Student or Prof, otherwise idk.
	if (strcmp(type,"S")==0)	
	{
		strcpy(pir->info.stud.faculty,depFac);
		pir->info.stud.admyear = year;
	}	
	else if (strcmp(type,"P")==0)
	{
		strcpy(pir->info.prof.dept,depFac);
		pir->info.prof.hireyear = year;	
		pir->info.prof.tenured = strsep(&input, ",\n")[0];
	}

	//if the linkedList is empty we simply return the pir.
	if (pirp == NULL)
	{
		return pir;
	}
        	
	//check if less than the first id.
	if (id < atoi(pirp->id))
	{
		PersonalInfoRecord *temp = pirp;
		pirp = pir;
		pirp->next = temp;
		return pirp;
	}

	//Check if the ID of the element is the first element of the linked list and update the entries with our input.
	if ((pirp != NULL) && ( id == atoi(pirp->id)))
        {
               if (strcmp(name, "")==0) strcpy(pir->name,pirp->name);
               if (strcmp(depFac, "")==0) strcpy(pir->info.prof.dept,pirp->info.prof.dept);
               if (year == 0) pir->info.prof.hireyear = pirp->info.prof.hireyear;
               if ((strcmp(type,"P")==0) && (strcmp(&pir->info.prof.tenured,"")==0)) pir->info.prof.tenured = pirp->info.prof.tenured;
               pir->next = pirp->next;
                pirp = pir;

        }

	//Set our return value as the first pirp
	PersonalInfoRecord *firstPirp = pirp;

	//Iterate until we find the appropriate slot for the id.
	while (pirp->next != NULL && id > atoi(pirp->next->id))
	{
		pirp = pirp->next;
	}

	//Check if the pirp->next is the same id as the input id and update the entries with our input.
	if ((pirp->next != NULL) && ( id == atoi(pirp->next->id)))
	{

		if (strcmp(name, "")==0) strcpy(pir->name,pirp->next->name);  
		if (strcmp(depFac, "")==0) strcpy(pir->info.prof.dept,pirp->next->info.prof.dept);
		if (year == 0) pir->info.prof.hireyear = pirp->next->info.prof.hireyear;
		if ((strcmp(type,"P")==0) && (strcmp(&pir->info.prof.tenured,"")==0)) pir->info.prof.tenured = pirp->next->info.prof.tenured;
		pir->next = pirp->next->next;
		pirp->next = pir;
	
	}

	//Add the corresponding Id.
	else
	{
		PersonalInfoRecord *temp = pirp->next;
		pirp->next = pir;
		pir->next = temp;
	}

	return firstPirp;
}

//Function to delete an Entry from the linkedList.
PersonalInfoRecord* deleteEntry(char* idChar, PersonalInfoRecord *pirp)
{
	int id = atoi(idChar);

	//Check if our ID is the first element of the LinkedList
	if (strcmp(pirp->id,idChar)==0)
	{
		PersonalInfoRecord *temp = pirp->next;
		free(pirp);
		return temp;
	}
	
	//Set our return value as the first pirp
	PersonalInfoRecord *firstPirp = pirp;
	
	//loops until the pirp->next is the id, and then we replace it with the ->next->next.
	while (pirp->next != NULL && strcmp(idChar, pirp->next->id) != 0)
	{
		pirp = pirp->next;
	}

	//Remove the corresonding Id.
	PersonalInfoRecord *temp = pirp->next->next;
	free(pirp->next);
	pirp->next = temp;

	return firstPirp;


}

PersonalInfoRecord* printList(PersonalInfoRecord *pirp)
{
	//Check if the list is empty
	if(pirp == NULL) return pirp;
	
	PersonalInfoRecord *firstPirp = pirp;
        while (pirp != NULL)
	{ 
		//Check whether Prof or Student.
		if (strcmp(&pirp->ptype,"P") == 0)
		{
			printf("%s,%c,%s,%s,%d,%c\n", pirp->id, pirp->ptype, pirp->name, pirp->info.prof.dept, pirp->info.prof.hireyear, pirp->info.prof.tenured);
		}
		else
		{
			printf("%s,%c,%s,%s,%d\n", pirp->id, pirp->ptype, pirp->name, pirp->info.stud.faculty, pirp->info.stud.admyear);
		}
		pirp = pirp->next;
	}
		
	return firstPirp;

}

//Method to write Linked List in the file that was included when the program was executed.
int writeList(PersonalInfoRecord *pirp, char* filename)
{
	FILE *file;
	file = fopen(filename,"w");

	//Check if the file format is adequate.
	if (file  == NULL)
	{
		fprintf(stderr, "Error, unable to open %s for writing.", filename);	
		
		while ( pirp != NULL)
		{
			PersonalInfoRecord *temp = pirp->next;
        		free(pirp);
       			pirp = temp;
		}	
		return 3;
	}
	
	while( pirp != NULL)
	{
		//Basically the same as the printList function but we use fprintf instead so we print it in the file.
		if (strcmp(&pirp->ptype,"P") == 0)
		{
			fprintf(file,"%s,%c,%s,%s,%d,%c\n", pirp->id, pirp->ptype, pirp->name, pirp->info.prof.dept, pirp->info.prof.hireyear, pirp->info.prof.tenured);
		}
		else
		{
			fprintf(file,"%s,%c,%s,%s,%d\n", pirp->id, pirp->ptype, pirp->name, pirp->info.stud.faculty, pirp->info.stud.admyear);
		}
		PersonalInfoRecord *temp = pirp->next;
		free(pirp);
		pirp = temp;
	}
	fclose(file);
	return 0;
}
// The main of your application
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		puts("Error, please pass the database filename as the argument.");
		return 1;
	}
	
	char inputbuffer[100], *input; // to store each input line;
	PersonalInfoRecord *pirp = NULL;
	char *filename = argv[1]; 

	while (fgets(input=inputbuffer, 100, stdin) != NULL) // Get each input line.
	{
		
		if(strncmp(input, "END", 3) == 0) 
		{
			return writeList(pirp, filename);
		}	// We are asked to terminate.
		
		//LIST command to list out the elements of the LinkedList.
		else if(strncmp(input, "LIST", 4) == 0)
		{
			pirp = printList(pirp);
			continue;
		}
	
		int field = 1; char*fielddata;

		fielddata = strsep(&input, ",\n");
                
		// Check if first character is I, to see if we are adding an entry.
		if (strcmp(fielddata, "I") == 0)
		{
			pirp = addEntry(input,pirp);
		}
		
		// Check if first character is D, to see if we are deleting the entry.
		else if (strcmp(fielddata, "D") == 0)
		{
			pirp = deleteEntry(strsep(&input, ",\n"), pirp);
			
		}


	}	

	return 0; // Appropriate return code from this program.
	
}
