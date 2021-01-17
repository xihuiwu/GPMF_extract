import pandas as pd

'''
Helper function to read CSV file
'''
def readCsv(filename, cols):
    df = pd.read_csv(filename, header=None, usecols = [i for i in range(cols)])
    data = []
    timestamp = []

    for _,row in df.iterrows():
        for i in range(len(row)):
            if i == 0:
                timestamp.append(row[i])
            else:
                data.append(row[i])
    return data, timestamp

'''
Helper function to filter out high frequency data
alpha is the smoothing factor which lies between 0 and 1
'''
def low_pass(data, alpha):
	res = []
	res.append(data[0])
	for i in range(1:len(data)):
		temp = alpha*data[i] + (1 - alpha)*res[i-1]
		res.append(temp)
	return res