/* ///////////////////////////////////////////////////////////////////// */
/*!
  \file
  \brief MHD blast wave.

  The MHD blast wave problem has been specifically designed to show the scheme ability to handle strong shock waves propagating in highly magnetized
  environments.
  Depending on the strength of the magnetic field, it can become a rather
  arduous test leading to unphysical densities or pressures if the
  divergence-free condition is not adequately controlled and the numerical
  scheme does not introduce proper dissipation across curved shock fronts.

  In this example the initial conditions consists of a static medium with
  uniform density  \f$ \rho = 1 \f$ while pressure and magnetic field
  are given by
  \f[
     p = \left\{\begin{array}{ll}
           p_{\rm in}   & \qquad\mathrm{for}\quad  r < r_0 \\ \noalign{\medskip}
           p_{\rm out}  & \qquad\mathrm{otherwise} \\
     \end{array}\right.\,;\qquad
     \vec{B} = B_0\left(  \sin\theta\cos\phi\hvec{x}
                        + \sin\theta\sin\phi\hvec{y}
                        + \cos\theta\hvec{z}\right)
  \f]
  The values \f$p_{\rm in},\, p_{\rm out},\, B_0,\, \theta,\, \phi,\, r_0\f$
  are control parameters that can be changed from \c pluto.ini using,
  respectively,

  -# <tt>g_inputParam[P_IN]</tt>
  -# <tt>g_inputParam[P_OUT]</tt>
  -# <tt>g_inputParam[BMAG]</tt>
  -# <tt>g_inputParam[THETA]</tt>
  -# <tt>g_inputParam[PHI]</tt>
  -# <tt>g_inputParam[RADIUS]</tt>

  The over-pressurized region drives a blast wave delimited by an outer
  fast forward shock propagating (nearly) radially while magnetic field
  lines pile up behind the shock thus building a region of higher magnetic
  pressure.
  In these regions the shock becomes magnetically dominated and only weakly
  compressive (\f$\delta\rho/\rho\sim 1.2\f$ in both cases).
  The inner structure is delimited by an oval-shaped slow shock adjacent to a
  contact discontinuity and the  two fronts tend to blend together as the
  propagation becomes perpendicular to the field lines.
  The magnetic energy increases behind the fast shock and decreases
  downstream of the slow shock.
  The resulting explosion becomes highly anisotropic and magnetically confined.

  The available configurations are taken by collecting different setups
  available in literature:

  <CENTER>
  Conf.| GEOMETRY  |DIM| T. STEP.|INTERP.  |divB| BCK_FIELD | Ref
  -----|-----------|---|---------| --------| ---|-----------|----------------
   #01 |CARTESIAN  | 2 |  RK2    |LINEAR   | CT |   NO      |[BS99]
   #02 |CARTESIAN  | 3 |  RK2    |LINEAR   | CT |   NO      |[Z04]
   #03 |CYLINDRICAL| 2 |  RK2    |LINEAR   | CT |   NO      |[Z04] (*)
   #04 |CYLINDRICAL| 2 |  RK2    |LINEAR   | CT |   YES     |[Z04] (*)
   #05 |CARTESIAN  | 3 |  RK2    |LINEAR   | CT |   YES     |[Z04]
   #06 |CARTESIAN  | 3 |  ChTr   |PARABOLIC| CT |   NO      |[GS08],[MT10]
   #07 |CARTESIAN  | 3 |  ChTr   |LINEAR   | CT |   NO      |[GS08],[MT10]
   #08 |CARTESIAN  | 2 |  ChTr   |LINEAR   | GLM|   NO      |[MT10] (2D version)
   #09 |CARTESIAN  | 3 |  ChTr   |LINEAR   | GLM|   NO      |[GS08],[MT10]
   #10 |CARTESIAN  | 3 |  RK2    |LINEAR   | CT |   YES     |[Z04]
   #11 |CARTESIAN  | 3 |  ChTr   |LINEAR   |EGLM|   NO      |[MT10] (**)
  </CENTER>

  (*)  Setups are in different coordinates and with different orientation
       of magnetic field using constrained-transport MHD.
  (**) second version in sec. 4.7

  The snapshot below show the solution for configuration #11.

  This setup also works with the \c BACKGROUND_FIELD spliting.
  In this case the initial magnetic field is assigned in the
  ::BackgroundField() function while the Init() function is used to
  initialize the deviation to 0.

  \image html mhd_blast-rho.11.jpg "Density contours at the end of simulation (conf. #11)"
  \image html mhd_blast-prs.11.jpg "Pressure contour at the end of simulation (conf. #11)"
  \image html mhd_blast-pm.11.jpg "Magnetic pressure contours at the end of simulation (conf. #11)"

  \authors A. Mignone (mignone@ph.unito.it)
  \date    Sept 24, 2014

  \b References: \n
     - [BS99]: "A Staggered Mesh Algorithm using High Order ...",
       Balsara \& Spicer, JCP (1999) 149, 270 (Sec 3.2)
     - [GS08]: "An unsplit Godunov method for ideal MHD via constrained
        transport in three dimensions", Gardiner \& Stone, JCP (2008) 227, 4123
        (Sec 5.5)
     - [MT10] "A second-order unsplit Godunov scheme for cell-centered MHD:
       The CTU-GLM scheme", Mignone \& Tzeferacos, JCP (2010) 229, 2117
       (Sec 4.7)
     - [Z04]: "A central-constrained transport scheme for ideal
       magnetohydrodynamics", Ziegler, JCP (2004) 196, 393 (Sec. 4.6)
*/
/* ///////////////////////////////////////////////////////////////////// */
#include "pluto.h"
#include "math.h"
#include "time.h"
#include "stdlib.h"
#include "stdio.h"
/* ********************************************************************* */
void Init (double *us, double x1, double x2, double x3)
/*
 *
 *
 *
 *********************************************************************** */
{
  static int first_call = 1;
  double r, theta, phi, B0, E_ej, M_ej, R_ej, n_h, w_c, s, n, n_ISM, r_c, T, u, g_gamma;
  double fnc, alpha, v_ej, t, rho_ch, R_ch, eta, ph, l, up, down;
  E_ej    = g_inputParam[E_EJ];         //爆发能量
  M_ej    = g_inputParam[M_EJ];         //爆发质量
  R_ej    = g_inputParam[R_EJ];         //爆发半径
  n_h     = g_inputParam[N_H];          //氢数密度
  u       = g_inputParam[U_AM];            //介质总体数密度，U是平均分子权重
  w_c     = g_inputParam[W_C];          //激波区质量与爆发总质量之比
  n       = g_inputParam[N_PI];            //激波区密度幂指数
  s       = g_inputParam[S_PI];            //激波区速度幂指数
  n_ISM   = n_h*u;                     //激波前均匀介质区数密度
  T       = g_inputParam[Temp];         //初始温度
  g_gamma = g_inputParam[GAMMA];        //绝热系数

  r_c     = R_ej*w_c;
  fnc     = 3.0/4.0/CONST_PI*(1.0-n/3.0)/(1.0-n/3.0*pow(w_c,3.0-n));
  alpha   = (3.0-n)/(5.0-n)*(pow(w_c,n-5.0)-n/5.0)/(pow(w_c,n-3.0)-n/3.0)*pow(w_c,2);
  v_ej    = pow(E_ej/(M_ej*alpha*0.5),0.5);    
  t       = R_ej/v_ej;   
  rho_ch  = M_ej/pow(R_ej,3);    
  R_ch    = pow(M_ej,1.0/3.0)*pow(n_ISM,-1.0/3.0);
  
  ph      = 1.1;  //n=0 
  l       = 0.343;
//  up      = 1 + (n-3)/3*pow(phi/l*fnc,0.5)*pow(R_ej,1.5)
//  down    = 1 + (n/3)*pow(phi/l*fnc,0.5)*pow(R_ej,1.5)
//  eta     = up/down
   
//  rho_c   = (1-eta)*M_ej/(4/3*CONST_PI*pow(r_c,3)); //激波后均匀介质区密度
//  C       = 2.0*CONST_PI*pow(r_c,n)*rho_c*pow(R_ej,-2*s);
//  index   = 2*s+3-n;
//  
//  part1   = C/pow(r_c,n)*pow(r_c,2*s+3)/(2*s+3);
//  part2   = C*(pow(R_ej,index)-pow(r_c,index))/index;
//  v0      = pow(E_ej,0.5)*pow(part1+part2,-0.5);

  r = D_EXPAND(x1*x1, + x2*x2, + x3*x3);
  r = sqrt(r);

  us[RHO] = n_ISM;
  us[VX1] = 0.0;
  us[VX2] = 0.0;
  us[VX3] = 0.0;
  us[PRS] = n_ISM*CONST_kB*T/1.67e-6;

  #if ADD_TURBULENCE == YES
  if (first_call){
    int k, input_var[200];
    for (k = 0; k< 200; k++) input_var[k] = -1;
    input_var[0] = RHO;
    input_var[1] = BX1;
    input_var[2] = BX2;
    input_var[3] = BX3;         
    input_var[4] = -1;
    InputDataSet ("./grid0.out",input_var);
    InputDataRead("./rho0.dbl"," ");
    first_call = 0;
  }
  InputDataInterpolate(us, x1, x2, x3);  /* -- interpolate density from
                                              input data file -- */
  #endif
/*
  if (r > 2.5 && r <= 3)
  {
    us[RHO] = 60*pow(r/r_c,0.0);
  }
*/

  if (r <= r_c && r != 0)
  {
    up      = 1.0 + (n-3.0)/3.0*pow(ph/l*fnc,0.5)*pow(r/R_ch,1.5);
    down    = 1.0 + (n/3.0)*pow(ph/l*fnc,0.5)*pow(r/R_ch,1.5);
    eta     = up/down;
//    eta     = 1.0;
    us[RHO] = rho_ch*fnc*pow(w_c,-n);
    us[VX1] = (x1/t)*eta;
    us[VX2] = (x2/t)*eta;
    us[VX3] = (x3/t)*eta;
    us[PRS] = rho_ch*fnc*pow(w_c,-n)*CONST_kB*T/1.67e-6;
  }

  if (r >  r_c && r <= R_ej)
  {
   // us[RHO] = a[(int) fabs(x1*x2*100)]*rho_c*pow(r/r_c,-n);
    up      = 1.0 + (n-3.0)/3.0*pow(ph/l*fnc,0.5)*pow(r/R_ch,1.5);
    down    = 1.0 + (n/3.0)*pow(ph/l*fnc,0.5)*pow(r/R_ch,1.5);
    eta     = up/down;
//    eta     = 1.0;
    us[RHO] = rho_ch*fnc*pow(r/R_ej,-n);
    us[VX1] = (x1/t)*eta;
    us[VX2] = (x2/t)*eta;
    us[VX3] = (x3/t)*eta;
    us[PRS] = rho_ch*fnc*pow(r/R_ej,-n)*CONST_kB*T/1.67e-6;
  }

//  printf("%e\n",eta);
  //theta = g_inputParam[THETA]*CONST_PI/180.0;
  //phi   =   g_inputParam[PHI]*CONST_PI/180.0;
  B0    = g_inputParam[BMAG];

  //us[BX1] = B0*sin(theta)*cos(phi);
  //us[BX2] = B0*sin(theta)*sin(phi);
  //us[BX3] = 0.0;

  #if GEOMETRY == CARTESIAN
   us[AX1] = 0.0;
   us[AX2] =  us[BX3]*x1;
   us[AX3] = -us[BX2]*x1 + us[BX1]*x2;
  #elif GEOMETRY == CYLINDRICAL
   us[AX1] = us[AX2] = 0.0;
   us[AX3] = 0.5*us[BX2]*x1;
  #endif

  #if BACKGROUND_FIELD == YES
   us[BX1] = us[BX2] = us[BX3] =
   us[AX1] = us[AX2] = us[AX3] = 0.0;
  #endif

}
/* ********************************************************************* */
void Analysis (const Data *d, Grid *grid)
/*
 *
 *
 *********************************************************************** */
{

}
/* ********************************************************************* */
void UserDefBoundary (const Data *d, RBox *box, int side, Grid *grid)
/*
 *
 *********************************************************************** */
{
}
#if BACKGROUND_FIELD == YES
/* ********************************************************************* */
void BackgroundField (double x1, double x2, double x3, double *B0)
/*!
 * Define the component of a static, curl-free background
 * magnetic field.
 *
 *********************************************************************** */
{
/*
  static int first_call = 1;
  double theta, phi;
  static double sth,cth,sphi,cphi;

  if (first_call){
    theta = g_inputParam[THETA]*CONST_PI/180.0;
    phi   =   g_inputParam[PHI]*CONST_PI/180.0;
    sth   = sin(theta);
    cth   = cos(theta);
    sphi  = sin(phi);
    cphi  = cos(phi);
    first_call = 0;
  }
  EXPAND(B0[IDIR] = g_inputParam[BMAG]*sth*cphi; ,
         B0[JDIR] = g_inputParam[BMAG]*sth*sphi; ,
         B0[KDIR] = g_inputParam[BMAG]*cth;)

/*
  theta = g_inputParam[THETA]*CONST_PI/180.0;
  phi   =   g_inputParam[PHI]*CONST_PI/180.0;

  B0[IDIR] = g_inputParam[BMAG]*sin(theta)*cos(phi);
  B0[JDIR] = g_inputParam[BMAG]*sin(theta)*sin(phi);
  B0[KDIR] = g_inputParam[BMAG]*cos(theta);
*/

}
#endif
