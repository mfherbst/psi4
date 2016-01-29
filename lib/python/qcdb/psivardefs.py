try:
    from collections import OrderedDict
except ImportError:
    from .oldpymodules import OrderedDict

def sapt_psivars():
    """Returns dictionary of PsiVariable definitions.

    """
    pv1 = OrderedDict()
    pv1['SAPT EXCHSCAL'] = {'func': lambda x: 1.0 if x[0] < 1.0e-5 else x[0] / x[1], 'args': ['SAPT EXCH10 ENERGY', 'SAPT EXCH10(S^2) ENERGY']}  # special treatment in pandas
    pv1['SAPT EXCHSCAL3'] = {'func': lambda x: x[0] ** 3, 'args': ['SAPT EXCHSCAL']}
    pv1['SAPT HF(2) ALPHA=0.0 ENERGY'] = {'func': lambda x: x[0] - (x[1] + x[2] + x[3] + x[4]),
                                          'args': ['SAPT HF TOTAL ENERGY', 'SAPT ELST10,R ENERGY', 'SAPT EXCH10 ENERGY',
                                                   'SAPT IND20,R ENERGY', 'SAPT EXCH-IND20,R ENERGY']}
    pv1['SAPT HF(2) ENERGY'] = {'func': lambda x: x[1] + (1.0 - x[0]) * x[2],
                                'args': ['SAPT EXCHSCAL', 'SAPT HF(2) ALPHA=0.0 ENERGY', 'SAPT EXCH-IND20,R ENERGY']}
    pv1['SAPT HF(3) ENERGY'] = {'func': lambda x: x[1] - (x[2] + x[0] * x[3]),
                                'args': ['SAPT EXCHSCAL', 'SAPT HF(2) ENERGY', 'SAPT IND30,R ENERGY', 'SAPT EXCH-IND30,R ENERGY']}
    pv1['SAPT MP2(2) ENERGY'] = {'func': lambda x: x[1] - (x[2] + x[3] + x[4] + x[0] * (x[5] + x[6] + x[7] + x[8])),
                                 'args': ['SAPT EXCHSCAL', 'SA MP2 CORRELATION ENERGY', 'SAPT ELST12,R ENERGY',  # MP2 CORRELATION ENERGY renamed here from pandas since this is IE
                                          'SAPT IND22 ENERGY', 'SAPT DISP20 ENERGY', 'SAPT EXCH11(S^2) ENERGY',
                                          'SAPT EXCH12(S^2) ENERGY', 'SAPT EXCH-IND22 ENERGY', 'SAPT EXCH-DISP20 ENERGY']}
    pv1['SAPT MP2(3) ENERGY'] = {'func': lambda x: x[1] - (x[2] + x[0] * x[3]),
                                 'args': ['SAPT EXCHSCAL', 'SAPT MP2(2) ENERGY', 'SAPT IND-DISP30 ENERGY', 'SAPT EXCH-IND-DISP30 ENERGY']}
    pv1['SAPT MP4 DISP'] = {'func': lambda x: x[0] * x[1] + x[2] + x[3] + x[4] + x[5],
                            'args': ['SAPT EXCHSCAL', 'SAPT EXCH-DISP20 ENERGY', 'SAPT DISP20 ENERGY',
                                     'SAPT DISP21 ENERGY', 'SAPT DISP22(SDQ) ENERGY', 'SAPT EST.DISP22(T) ENERGY']}
    pv1['SAPT CCD DISP'] = {'func': lambda x: x[0] * x[1] + x[2] + x[3] + x[4],
                            'args': ['SAPT EXCHSCAL', 'SAPT EXCH-DISP20 ENERGY', 'SAPT DISP2(CCD) ENERGY',
                                     'SAPT DISP22(S)(CCD) ENERGY', 'SAPT EST.DISP22(T)(CCD) ENERGY']}
    pv1['SAPT0 ELST ENERGY'] = {'func': sum, 'args': ['SAPT ELST10,R ENERGY']}
    pv1['SAPT0 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT EXCH10 ENERGY']}
    pv1['SAPT0 INDC ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3],
                                'args': ['SAPT EXCHSCAL', 'SAPT HF(2) ENERGY', 'SAPT IND20,R ENERGY', 'SAPT EXCH-IND20,R ENERGY']}
    pv1['SAPT0 DISP ENERGY'] = {'func': lambda x: x[0] * x[1] + x[2],
                                'args': ['SAPT EXCHSCAL', 'SAPT EXCH-DISP20 ENERGY', 'SAPT DISP20 ENERGY']}
    pv1['SAPT0 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT0 ELST ENERGY', 'SAPT0 EXCH ENERGY', 'SAPT0 INDC ENERGY', 'SAPT0 DISP ENERGY']}
    pv1['SSAPT0 ELST ENERGY'] = {'func': sum, 'args': ['SAPT0 ELST ENERGY']}
    pv1['SSAPT0 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT0 EXCH ENERGY']}
    pv1['SSAPT0 INDC ENERGY'] = {'func': lambda x: x[1] + (x[0] - 1.0) * x[2],
                                 'args': ['SAPT EXCHSCAL3', 'SAPT0 INDC ENERGY', 'SAPT EXCH-IND20,R ENERGY']}
    pv1['SSAPT0 DISP ENERGY'] = {'func': lambda x: x[0] * x[1] + x[2],
                                 'args': ['SAPT EXCHSCAL3', 'SAPT EXCH-DISP20 ENERGY', 'SAPT DISP20 ENERGY']}
    pv1['SSAPT0 TOTAL ENERGY'] = {'func': sum, 'args': ['SSAPT0 ELST ENERGY', 'SSAPT0 EXCH ENERGY', 'SSAPT0 INDC ENERGY', 'SSAPT0 DISP ENERGY']}
    pv1['SCS-SAPT0 ELST ENERGY'] = {'func': sum, 'args': ['SAPT0 ELST ENERGY']}
    pv1['SCS-SAPT0 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT0 EXCH ENERGY']}
    pv1['SCS-SAPT0 INDC ENERGY'] = {'func': sum, 'args': ['SAPT0 INDC ENERGY']}
    pv1['SCS-SAPT0 DISP ENERGY'] = {'func': lambda x: x[0] * (x[1] + x[2]) + x[3] * (x[4] + x[5]),
                                    'args': [0.66, 'SAPT EXCH-DISP20(SS) ENERGY', 'SAPT DISP20(SS) ENERGY',
                                             1.2, 'SAPT EXCH-DISP20(OS) ENERGY', 'SAPT DISP20(OS) ENERGY']}  # note no xs for SCS disp
    pv1['SCS-SAPT0 TOTAL ENERGY'] = {'func': sum, 'args': ['SCS-SAPT0 ELST ENERGY', 'SCS-SAPT0 EXCH ENERGY', 'SCS-SAPT0 INDC ENERGY', 'SCS-SAPT0 DISP ENERGY']}
    pv1['SAPT2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT ELST10,R ENERGY', 'SAPT ELST12,R ENERGY']}
    pv1['SAPT2 EXCH ENERGY'] = {'func': lambda x: x[1] + x[0] * (x[2] + x[3]),
                                'args': ['SAPT EXCHSCAL', 'SAPT EXCH10 ENERGY', 'SAPT EXCH11(S^2) ENERGY', 'SAPT EXCH12(S^2) ENERGY']}
    pv1['SAPT2 INDC ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3] + x[4] + x[0] * x[5],
                                'args': ['SAPT EXCHSCAL', 'SAPT HF(2) ENERGY', 'SAPT IND20,R ENERGY', 'SAPT EXCH-IND20,R ENERGY',
                                         'SAPT IND22 ENERGY', 'SAPT EXCH-IND22 ENERGY']}
    pv1['SAPT2 DISP ENERGY'] = {'func': lambda x: x[0] * x[1] + x[2],
                                'args': ['SAPT EXCHSCAL', 'SAPT EXCH-DISP20 ENERGY', 'SAPT DISP20 ENERGY']}
    pv1['SAPT2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2 ELST ENERGY', 'SAPT2 EXCH ENERGY', 'SAPT2 INDC ENERGY', 'SAPT2 DISP ENERGY']}
    pv1['SAPT2+ ELST ENERGY'] = {'func': sum, 'args': ['SAPT ELST10,R ENERGY', 'SAPT ELST12,R ENERGY']}
    pv1['SAPT2+ EXCH ENERGY'] = {'func': lambda x: x[1] + x[0] * (x[2] + x[3]),
                                 'args': ['SAPT EXCHSCAL', 'SAPT EXCH10 ENERGY', 'SAPT EXCH11(S^2) ENERGY', 'SAPT EXCH12(S^2) ENERGY']}
    pv1['SAPT2+ INDC ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3] + x[4] + x[0] * x[5],
                                 'args': ['SAPT EXCHSCAL', 'SAPT HF(2) ENERGY', 'SAPT IND20,R ENERGY', 'SAPT EXCH-IND20,R ENERGY',
                                          'SAPT IND22 ENERGY', 'SAPT EXCH-IND22 ENERGY']}
    pv1['SAPT2+ DISP ENERGY'] = {'func': sum, 'args': ['SAPT MP4 DISP']}
    pv1['SAPT2+ TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+ ELST ENERGY', 'SAPT2+ EXCH ENERGY', 'SAPT2+ INDC ENERGY', 'SAPT2+ DISP ENERGY']}
    pv1['SAPT2+(CCD) ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+ ELST ENERGY']}
    pv1['SAPT2+(CCD) EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+ EXCH ENERGY']}
    pv1['SAPT2+(CCD) INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+ INDC ENERGY']}
    pv1['SAPT2+(CCD) DISP ENERGY'] = {'func': sum, 'args': ['SAPT CCD DISP']}
    pv1['SAPT2+(CCD) TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+(CCD) ELST ENERGY', 'SAPT2+(CCD) EXCH ENERGY', 'SAPT2+(CCD) INDC ENERGY', 'SAPT2+(CCD) DISP ENERGY']}
    pv1['SAPT2+DMP2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+ ELST ENERGY']}
    pv1['SAPT2+DMP2 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+ EXCH ENERGY']}
    pv1['SAPT2+DMP2 INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+ INDC ENERGY', 'SAPT MP2(2) ENERGY']}
    pv1['SAPT2+DMP2 DISP ENERGY'] = {'func': sum, 'args': ['SAPT2+ DISP ENERGY']}
    pv1['SAPT2+DMP2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+DMP2 ELST ENERGY', 'SAPT2+DMP2 EXCH ENERGY', 'SAPT2+DMP2 INDC ENERGY', 'SAPT2+DMP2 DISP ENERGY']}
    pv1['SAPT2+(CCD)DMP2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+ ELST ENERGY']}
    pv1['SAPT2+(CCD)DMP2 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+ EXCH ENERGY']}
    pv1['SAPT2+(CCD)DMP2 INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+DMP2 INDC ENERGY']}
    pv1['SAPT2+(CCD)DMP2 DISP ENERGY'] = {'func': sum, 'args': ['SAPT2+(CCD) DISP ENERGY']}
    pv1['SAPT2+(CCD)DMP2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+(CCD)DMP2 ELST ENERGY', 'SAPT2+(CCD)DMP2 EXCH ENERGY', 'SAPT2+(CCD)DMP2 INDC ENERGY', 'SAPT2+(CCD)DMP2 DISP ENERGY']}
    pv1['SAPT2+(3) ELST ENERGY'] = {'func': sum, 'args': ['SAPT ELST10,R ENERGY', 'SAPT ELST12,R ENERGY', 'SAPT ELST13,R ENERGY']}
    pv1['SAPT2+(3) EXCH ENERGY'] = {'func': lambda x: x[1] + x[0] * (x[2] + x[3]),
                                    'args': ['SAPT EXCHSCAL', 'SAPT EXCH10 ENERGY', 'SAPT EXCH11(S^2) ENERGY', 'SAPT EXCH12(S^2) ENERGY']}
    pv1['SAPT2+(3) INDC ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3] + x[4] + x[0] * x[5],
                                    'args': ['SAPT EXCHSCAL', 'SAPT HF(2) ENERGY', 'SAPT IND20,R ENERGY', 'SAPT EXCH-IND20,R ENERGY',
                                             'SAPT IND22 ENERGY', 'SAPT EXCH-IND22 ENERGY']}
    pv1['SAPT2+(3) DISP ENERGY'] = {'func': sum, 'args': ['SAPT MP4 DISP', 'SAPT DISP30 ENERGY']}
    pv1['SAPT2+(3) TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) ELST ENERGY', 'SAPT2+(3) EXCH ENERGY', 'SAPT2+(3) INDC ENERGY', 'SAPT2+(3) DISP ENERGY']}
    pv1['SAPT2+(3)(CCD) ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) ELST ENERGY']}
    pv1['SAPT2+(3)(CCD) EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) EXCH ENERGY']}
    pv1['SAPT2+(3)(CCD) INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) INDC ENERGY']}
    pv1['SAPT2+(3)(CCD) DISP ENERGY'] = {'func': sum, 'args': ['SAPT CCD DISP', 'SAPT DISP30 ENERGY']}
    pv1['SAPT2+(3)(CCD) TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+(3)(CCD) ELST ENERGY', 'SAPT2+(3)(CCD) EXCH ENERGY', 'SAPT2+(3)(CCD) INDC ENERGY', 'SAPT2+(3)(CCD) DISP ENERGY']}
    pv1['SAPT2+(3)DMP2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) ELST ENERGY']}
    pv1['SAPT2+(3)DMP2 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) EXCH ENERGY']}
    pv1['SAPT2+(3)DMP2 INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) INDC ENERGY', 'SAPT MP2(2) ENERGY']}
    pv1['SAPT2+(3)DMP2 DISP ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) DISP ENERGY']}
    pv1['SAPT2+(3)DMP2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+(3)DMP2 ELST ENERGY', 'SAPT2+(3)DMP2 EXCH ENERGY', 'SAPT2+(3)DMP2 INDC ENERGY', 'SAPT2+(3)DMP2 DISP ENERGY']}
    pv1['SAPT2+(3)(CCD)DMP2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) ELST ENERGY']}
    pv1['SAPT2+(3)(CCD)DMP2 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+(3) EXCH ENERGY']}
    pv1['SAPT2+(3)(CCD)DMP2 INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+(3)DMP2 INDC ENERGY']}
    pv1['SAPT2+(3)(CCD)DMP2 DISP ENERGY'] = {'func': sum, 'args': ['SAPT2+(3)(CCD) DISP ENERGY']}
    pv1['SAPT2+(3)(CCD)DMP2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+(3)(CCD)DMP2 ELST ENERGY', 'SAPT2+(3)(CCD)DMP2 EXCH ENERGY', 'SAPT2+(3)(CCD)DMP2 INDC ENERGY', 'SAPT2+(3)(CCD)DMP2 DISP ENERGY']}
    pv1['SAPT2+3 ELST ENERGY'] = {'func': sum, 'args': ['SAPT ELST10,R ENERGY', 'SAPT ELST12,R ENERGY', 'SAPT ELST13,R ENERGY']}
    pv1['SAPT2+3 EXCH ENERGY'] = {'func': lambda x: x[1] + x[0] * (x[2] + x[3]),
                                  'args': ['SAPT EXCHSCAL', 'SAPT EXCH10 ENERGY', 'SAPT EXCH11(S^2) ENERGY', 'SAPT EXCH12(S^2) ENERGY']}
    pv1['SAPT2+3 INDC ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3] + x[4] + x[0] * x[5] + x[6] + x[0] * x[7],
                                  'args': ['SAPT EXCHSCAL', 'SAPT HF(3) ENERGY', 'SAPT IND20,R ENERGY', 'SAPT EXCH-IND20,R ENERGY',
                                           'SAPT IND22 ENERGY', 'SAPT EXCH-IND22 ENERGY', 'SAPT IND30,R ENERGY', 'SAPT EXCH-IND30,R ENERGY']}
    pv1['SAPT2+3 DISP ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3] + x[4] + x[0] * x[5],
                                  'args': ['SAPT EXCHSCAL', 'SAPT MP4 DISP', 'SAPT DISP30 ENERGY', 'SAPT EXCH-DISP30 ENERGY',
                                           'SAPT IND-DISP30 ENERGY', 'SAPT EXCH-IND-DISP30 ENERGY']}
    pv1['SAPT2+3 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+3 ELST ENERGY', 'SAPT2+3 EXCH ENERGY', 'SAPT2+3 INDC ENERGY', 'SAPT2+3 DISP ENERGY']}
    pv1['SAPT2+3(CCD) ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+3 ELST ENERGY']}
    pv1['SAPT2+3(CCD) EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+3 EXCH ENERGY']}
    pv1['SAPT2+3(CCD) INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+3 INDC ENERGY']}
    pv1['SAPT2+3(CCD) DISP ENERGY'] = {'func': lambda x: x[1] + x[2] + x[0] * x[3] + x[4] + x[0] * x[5],
                                       'args': ['SAPT EXCHSCAL', 'SAPT CCD DISP', 'SAPT DISP30 ENERGY', 'SAPT EXCH-DISP30 ENERGY',
                                                'SAPT IND-DISP30 ENERGY', 'SAPT EXCH-IND-DISP30 ENERGY']}
    pv1['SAPT2+3(CCD) TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+3(CCD) ELST ENERGY', 'SAPT2+3(CCD) EXCH ENERGY', 'SAPT2+3(CCD) INDC ENERGY', 'SAPT2+3(CCD) DISP ENERGY']}
    pv1['SAPT2+3DMP2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+3 ELST ENERGY']}
    pv1['SAPT2+3DMP2 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+3 EXCH ENERGY']}
    pv1['SAPT2+3DMP2 INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+3 INDC ENERGY', 'SAPT MP2(3) ENERGY']}
    pv1['SAPT2+3DMP2 DISP ENERGY'] = {'func': sum, 'args': ['SAPT2+3 DISP ENERGY']}
    pv1['SAPT2+3DMP2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+3DMP2 ELST ENERGY', 'SAPT2+3DMP2 EXCH ENERGY', 'SAPT2+3DMP2 INDC ENERGY', 'SAPT2+3DMP2 DISP ENERGY']}
    pv1['SAPT2+3(CCD)DMP2 ELST ENERGY'] = {'func': sum, 'args': ['SAPT2+3 ELST ENERGY']}
    pv1['SAPT2+3(CCD)DMP2 EXCH ENERGY'] = {'func': sum, 'args': ['SAPT2+3 EXCH ENERGY']}
    pv1['SAPT2+3(CCD)DMP2 INDC ENERGY'] = {'func': sum, 'args': ['SAPT2+3DMP2 INDC ENERGY']}
    pv1['SAPT2+3(CCD)DMP2 DISP ENERGY'] = {'func': sum, 'args': ['SAPT2+3(CCD) DISP ENERGY']}
    pv1['SAPT2+3(CCD)DMP2 TOTAL ENERGY'] = {'func': sum, 'args': ['SAPT2+3(CCD)DMP2 ELST ENERGY', 'SAPT2+3(CCD)DMP2 EXCH ENERGY', 'SAPT2+3(CCD)DMP2 INDC ENERGY', 'SAPT2+3(CCD)DMP2 DISP ENERGY']}

    return pv1