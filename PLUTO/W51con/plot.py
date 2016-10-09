import os
import sys
from numpy import *
from matplotlib.pyplot import *
import pyPLUTO as pp

wdir = './'
nlinf = pp.nlast_info(w_dir=wdir)

#D = pp.pload(nlinf['nlast'],w_dir=wdir) # Loading the data into a pload object D
D = pp.pload(10,w_dir=wdir)
#print D.rho.shape[0]

#f=open('rho.txt','r+')
#temp=f.read()
#temp=

I = pp.Image()
I.pldisplay(D, D.rho*(D.bx1**2+D.bx2**2)**2.0*(D.vx1**2+D.vx2**2)**0,x1=D.x1, \
            x2=D.x2,label1='x',label2='y',                                    \
            title=r'Flux [M=20 E=2 R=1 t=2.5 rho=20]',                         \
            cbar=(True,'vertical'))
#savefig('MHD_Blast.png') # Only to be saved as either .png or .jpg
show()
