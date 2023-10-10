from json.tool import main
import numpy as np
import matplotlib.pyplot as plt
from IPython.core.debugger import set_trace

np.random.seed(1234)

Bernoulli = lambda theta,x: theta**x * (1-theta)**(1-x)
theta_star = .4
Bernoulli(theta_star, 1)

n = 10                                                              #number of random samples you want to consider 
xn = np.random.rand(n) < theta_star                                 #Generates n element boolean array where elements are True with probability theta_star and otherwise False
xn = xn.astype(int)                                                 #to change the boolean array to intergers [0:False, 1:True]
print("observation {}".format(xn))
#Function to compute the log likelihood
#Note that you can either pass this function a scalar(always broadcastable) theta or a broadcastable(in data axis) theta to get likelihood value or values
#Also note that we added an extra dimension in xn to broadcast it along theta dimension
L = lambda theta: np.prod(Bernoulli(theta, xn[None,:]), axis=-1)
#we generate 100 evenly placed values of theta from 0 to 1
theta_vals = np.linspace(0,1,100)[:, None]                         #Note that we made the array broadcastable by adding an extra dimension for data
plt.plot(theta_vals, L(theta_vals), '-')
plt.xlabel(r"$\theta$")
plt.ylabel(r"Likelihood $L(\theta)$")
#plt.title(r"likelihood function Bernoulli("+str(xn.astype(int))+r"$|\theta)$")
plt.show()

