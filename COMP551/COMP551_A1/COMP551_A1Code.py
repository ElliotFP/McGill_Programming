import numpy as np
import pandas as pd

# MiniProject 1: Getting Started with Machine Learning
# Javier Pollak, Vanessa Amar and Elliot F. Poirier

#Create arrays for the values of the data 

#Dataset 1
data1 = pd.read_excel('/Users/elliotfp/Documents/Programming/COMP551/COMP551_A1/ENB2012_data.xlsx')
dataset1 = np.array(data1)
dataset1clean = np.unique(dataset1, axis=0)

#Dataset 2
dataset2 = pd.read_csv('/Users/elliotfp/Documents/Programming/COMP551/COMP551_A1/Qualitative_Bankruptcy.data.txt', header=None)
dataset2.columns = ['Industrial Risk','Management Risk','Financial Flexibility', 'Credibility', 'Competitiveness', 'Operating Risk', 'Class']
dataset2clean = dataset2.drop_duplicates(keep='first')

 
if __name__ == '__main__':

    #show average values and median for each of the attributes and responses in Dataset1
    print("***Dataset 1 Information*** \t")
    print("\t")

    avg1 = np.mean(dataset1, axis = 0)
    med1 = np.median(dataset1, axis = 0)
    i=1
    for avg in avg1:
        print("X" + str(i) + " mean : " + str(avg))        
        i+=1
    i=1
    for med in med1:
        print("X" + str(i) + " median : " + str(med))        
        i+=1

    #Check for missing values or duplicates
    unq, count = np.unique(dataset1, axis=0, return_counts=True)
    print("Duplicate rows:", unq[count>1])
    print("Number of duplicated rows:", np.sum(unq[count>1]))

    print("\t")

    #Show the number of each characteristic value for all the attributes and for the response in Dataset2 and duplicates as well as any missing values.
    print("***Dataset2 Information*** \t")
    print("\t")

    for column in dataset2.columns:
        print(f"Column '{column}':")
        print(dataset2[column].value_counts(), end='\n\n')
    
    #Check for any missing values or duplicates
    print("These are the duplicate rows : " + str(dataset2.duplicated().sum()))
    print("These rows have missing values : " + str(dataset2.isna().sum()))

    

