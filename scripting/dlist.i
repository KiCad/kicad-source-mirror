/* DLIST python iteration code, to allow standard iteration over DLIST */

%extend DLIST
{
	%pythoncode
	{
		class DLISTIter:
			def __init__(self,aList):
				self.last = aList
		
			def next(self):
			
				item = self.last
				try:
				  item = item.Get()
				except:
				  pass
				     
				if item is None: 
					raise StopIteration
				else:
					ret = None
					
					# first item in list has "Get" as a DLIST
					try:
						ret = self.last.Get()
					except: 
						ret = self.last #next items just not..
					
					self.last = self.last.Next()
					
					# when the iterated object can be casted down in inheritance, just do it..
					if 'Cast' in dir(ret):
						ret = ret.Cast()
						
					return ret
	
		def __iter__(self):
			return self.DLISTIter(self)
			
	}
}
