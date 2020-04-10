# -*- coding: utf-8 -*-
"""
Created on Wed Apr  1 13:37:00 2020

@author: abc
"""

import tushare as ts
#获取股票列表
def get_tickets():
	df = ts.get_stock_basics()
	return df.to_json(orient='table')

#start='2020-01-01', end='2020-12-31'
def get_ticket(code,startday,endday):
	df = ts.get_hist_data(code, ktype='D', start=startday, end=endday)
	return df.to_json(orient='table')

