

class wxPoint
{
public:
    int x, y;


    wxPoint(int xx, int yy);
    ~wxPoint();
    
    
    %extend {
        
        wxPoint __add__(const wxPoint& pt) {
            return *self + pt;
        }

        
        
        wxPoint __sub__(const wxPoint& pt) {
            return *self - pt;
        }


        
        void Set(long x, long y) {
            self->x = x;
            self->y = y;
        }

        
        PyObject* Get() {
            //wxPyBlock_t blocked = wxPyBeginBlockThreads();
            PyObject* tup = PyTuple_New(2);
            PyTuple_SET_ITEM(tup, 0, PyInt_FromLong(self->x));
            PyTuple_SET_ITEM(tup, 1, PyInt_FromLong(self->y));
            //wxPyEndBlockThreads(blocked);
            return tup;
        }
    }

    %pythoncode { 
    def __eq__(self,other):		 return (self.x==other.x and self.y==other.y)
    def __ne__(self,other):               return not (self==other)
    def __str__(self):                   return str(self.Get())
    def __repr__(self):                  return 'wx.Point'+str(self.Get())
    def __len__(self):                   return len(self.Get())
    def __getitem__(self, index):        return self.Get()[index]
    def __setitem__(self, index, val):
        if index == 0: self.x = val
        elif index == 1: self.y = val
        else: raise IndexError
    def __nonzero__(self):               return self.Get() != (0,0)
    __safe_for_unpickling__ = True
    }
};

