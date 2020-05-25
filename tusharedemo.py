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

#获取历史日K数据 start='2020-01-01', end='2020-12-31'
def get_ticket(code,startday,endday):
	df = ts.get_hist_data(code, ktype='D', start=startday, end=endday)
	return df.to_json(orient='table')

#获取历史全部(含复权)数据 start='2020-01-01', end='2020-12-31'
def get_ticket_all(code,startday,endday):
    df = ts.get_h_data(code, autype='qfq', start=startday, end=endday)
    return df.to_json(orient='table')

#获取历史分笔数据 day='2020-01-01'
def get_ticket_tt(code,day,endday):
    df = ts.get_tick_data(code, date=startday, src='tt')
    return df.to_json(orient='table')

#获取实时数据 codes='600848','000980','000981'
def get_ticket_quotes(codes):
    symbos = codes.split(",")
    df = ts.get_realtime_quotes(symbos)
    return df.to_json(orient='table')
