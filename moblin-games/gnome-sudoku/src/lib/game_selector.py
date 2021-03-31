import gtk, gobject, time
import sudoku, saver, sudoku_maker, random
from sudoku import DifficultyRating as DR
import sudoku_thumber
import gnomeprint
from gettext import gettext as _
from timer import format_time,format_date,format_friendly_date,format_time_compact
from defaults import *
from simple_debug import simple_debug
from colors import color_hex_to_float
from gtk_goodies import gconf_wrapper

def color_from_difficulty (diff):
    if diff < DR.easy_range[1]:
        if diff < DR.easy_range[1]/3: c='#8ae234' # green
        elif diff < 2*DR.easy_range[1]/3: c='#73d216'
        else: c='#4e9a06'
    elif diff < DR.medium_range[1]:
        span = DR.medium_range[1] - DR.easy_range[1]
        if diff < DR.medium_range[0]+(span/3): c='#204a87' # sky blue
        elif diff < DR.medium_range[0]+(2*(span/3)): c='#3465a4'
        else: c='#729fcf'
    elif diff < DR.hard_range[1]:
        span = DR.hard_range[1] - DR.medium_range[1]
        if diff < DR.hard_range[0] + span/3: c='#fcaf3e' # orange
        elif diff < DR.hard_range[0] + span*2/3: c='#f57900'
        else: c='#ce5c00'
    else:
        span = DR.very_hard_range[1] - DR.hard_range[1]
        if diff < DR.very_hard_range[0] + span/3:
            c='#ef2929' # scarlet red
        elif diff < DR.very_hard_range[0]+ span*2/3:
            c='#cc0000'
        else:
            c='#a40000'  
    #print 'diff=',diff,'color=',c
    return color_hex_to_float(c)

class NewOrSavedGameSelector (gconf_wrapper.GConfWrapper):

    NEW_GAME = 0
    SAVED_GAME = 1

    glade_file = os.path.join(GLADE_DIR,'select_game.glade')

    @simple_debug
    def __init__ (self, sudokuMaker=None, gconf = None):
        if gconf:
            gconf_wrapper.GConfWrapper.__init__(self,gconf)
        self.sudoku_maker = sudokuMaker or sudoku_maker.SudokuMaker()
    
    def setup_dialog (self):
        self.glade = gtk.glade.XML(self.glade_file)
        self.dialog = self.glade.get_widget('dialog1')
        self.dialog.set_default_response(gtk.RESPONSE_CANCEL)
        self.dialog.connect('close',self.close)
        self.dialog.hide()
        self.saved_game_view = self.glade.get_widget('savedGameIconView')
        self.saved_game_widgets = [
            self.saved_game_view,
            self.glade.get_widget('savedGameLabel')
            ]
        self.glade.get_widget('savedGameLabel').set_mnemonic_widget(
            self.saved_game_view
            )
        self.new_game_view = self.glade.get_widget('newGameIconView')
        self.glade.get_widget('newGameLabel').set_mnemonic_widget(
            self.new_game_view
            )
        self.make_new_game_model()
        self.new_game_view.set_model(self.new_game_model)
        self.new_game_view.set_markup_column(0)
        self.new_game_view.set_pixbuf_column(1)
        selected_puzzle = None
        self.make_saved_game_model()
        if len(self.saved_game_model)==0:
            for w in self.saved_game_widgets: w.hide()
        else:
            self.saved_game_model.set_sort_column_id(2,gtk.SORT_DESCENDING)
            self.saved_game_view.set_model(self.saved_game_model)
            self.saved_game_view.set_markup_column(0)
            self.saved_game_view.set_pixbuf_column(1)
        for view in self.saved_game_view, self.new_game_view:
            view.set_item_width(150)
            view.set_columns(4)
            view.set_spacing(12)
            view.set_selection_mode(gtk.SELECTION_SINGLE)
        self.saved_game_view.connect('item-activated',self.saved_item_activated_cb)
        self.new_game_view.connect('item-activated',self.new_item_activated_cb)

    @simple_debug
    def make_new_game_model (self):
        # Description, Pixbuf, Puzzle (str)
        self.new_game_model = gtk.ListStore(str,gtk.gdk.Pixbuf,str)        
        for cat in DR.ordered_categories:
            rng = DR.categories[cat]; label = DR.label_by_cat[cat]
            #puzzle,diff = self.sudoku_maker.get_new_puzzle(.01*random.randint(*[r*100 for r in rng]))
            #diff_val = diff.value
            puzzle,diff_val = self.sudoku_maker.get_puzzles(1,[cat],new=True)[0]
            #print 'Got new puzzle for ',cat,'difficulty:',diff
            grid = sudoku.sudoku_grid_from_string(puzzle).grid
            self.new_game_model.append(('<b><i>'+label+'</i></b>',
                                        sudoku_thumber.make_pixbuf(grid,
                                                                   None,
                                                                   color_from_difficulty(diff_val)
                                                                   ),
                                        puzzle
                                        ))

    @simple_debug
    def make_saved_game_model (self):
        # Description, Image, Last-Access time (for sorting), Puzzle (jar)
        self.saved_game_model = gtk.ListStore(str,gtk.gdk.Pixbuf,int,gobject.TYPE_PYOBJECT)
        t = saver.SudokuTracker()
        for g in t.list_saved_games():
            #print 'game',g
            game = g['game'].split('\n')[0]
            grid = sudoku.sudoku_grid_from_string(game)
            sr = sudoku.SudokuRater(grid.grid)
            sdifficulty = sr.difficulty()
            desc = "<b><i>%s</i></b>\n<span size='small'><i>Last played %s</i>\n<i>Played for %s.</i></span>"%(
                _("%s puzzle")%sdifficulty.value_string(),
                format_friendly_date(g['saved_at']),
                format_time(g['timer.tot_time'],round_at=15,friendly=True)
                )
            #print 'Adding to saved...',g
            self.saved_game_model.append((
                desc,
                sudoku_thumber.make_pixbuf(grid.grid,
                                           sudoku.sudoku_grid_from_string(g['game'].split('\n')[1].replace(' ','')).grid,
                                           color_from_difficulty(sdifficulty.value)
                                           ),
                g['saved_at'],
                g
                ))

    @simple_debug
    def new_item_activated_cb (self, iconview, path):
        self.play_game(iconview.get_model()[path][2])

    @simple_debug
    def saved_item_activated_cb (self, iconview, path):
        self.resume_game(iconview.get_model()[path][3])

    @simple_debug
    def resume_game (self, jar):
        self.puzzle = (self.SAVED_GAME, jar)
        self.dialog.emit('response',gtk.RESPONSE_OK)

    @simple_debug
    def play_game (self, puzzle):
        self.puzzle = (self.NEW_GAME,puzzle)
        self.dialog.emit('response',gtk.RESPONSE_OK)

    @simple_debug        
    def close (self):
        self.dialog.emit('response',gtk.RESPONSE_CLOSE)        

    @simple_debug
    def handle_response (self, response):
        #print 'handle_response',response
        if response==gtk.RESPONSE_OK:
            #print 'Returning ',self.puzzle
            return self.puzzle
        else:
            #print 'handle_response returning',None
            return None    

    def run_swallowed_dialog (self, swallower):
        self.setup_dialog()
        return self.handle_response(
            swallower.run_dialog(self.dialog)
            )

    def run_dialog (self):
        self.setup_dialog()
        return self.handle_response(self.dialog.run())

class GameSelector (gconf_wrapper.GConfWrapper):

    def __init__ (self, sudoku_tracker, gconf=None):
        self.sudoku_tracker = sudoku_tracker
        if gconf:
            gconf_wrapper.GConfWrapper.__init__(self,gconf)

    def setup_dialog (self):
        self.glade = gtk.glade.XML(self.glade_file)
        self.dialog = self.glade.get_widget('dialog1')
        self.dialog.set_default_response(gtk.RESPONSE_OK)
        self.dialog.hide()
        self.tv = self.glade.get_widget('treeview1')        
        self.setup_tree()

    def setup_up_tree (self): raise NotImplementedError
    def get_puzzle (self): raise NotImplementedError
    
    def run_dialog (self):
        self.setup_dialog()
        self.dialog.show()
        ret = self.dialog.run()
        self.dialog.hide()
        return self.handle_response(ret)

    def run_swallowed_dialog (self, swallower):
        self.setup_dialog()
        response = swallower.run_dialog(self.dialog)
        return self.handle_response(response)
    
    def handle_response (self, ret):
        if ret==gtk.RESPONSE_OK:
            return self.get_puzzle()
        else:
            return None

class GamePrinter (gconf_wrapper.GConfWrapper):

    glade_file = os.path.join(GLADE_DIR,'print_games.glade')

    initial_prefs = {'sudokus_per_page':2,
                     'print_multiple_sudokus_to_print':4,
                     'print_minimum_difficulty':0,
                     'print_maximum_difficulty':0.9,
                     'print_easy':True,
                     'print_medium':True,
                     'print_hard':True,
                     'print_very_hard':True,
                     }

    def __init__ (self, sudoku_maker, gconf):
        gconf_wrapper.GConfWrapper.__init__(self,gconf)
        self.sudoku_maker = sudoku_maker
        self.glade = gtk.glade.XML(self.glade_file)
        # Set up toggles...
        for key,wname in [('mark_printed_as_played','markAsPlayedToggle'),
                          ('print_already_played_games','includeOldGamesToggle'),
                          ('print_easy','easyCheckButton'),
                          ('print_medium','mediumCheckButton'),
                          ('print_hard','hardCheckButton'),
                          ('print_very_hard','very_hardCheckButton'),                          
                          ]:
            setattr(self,wname,self.glade.get_widget(wname))
            try: assert(getattr(self,wname))
            except: raise AssertionError('Widget %s does not exist'%wname)
            self.gconf_wrap_toggle(key,getattr(self,wname))
        self.sudokusToPrintSpinButton = self.glade.get_widget('sudokusToPrintSpinButton')
        self.sudokusPerPageSpinButton = self.glade.get_widget('sudokusPerPageSpinButton')
        for key,widg in [('print_multiple_sudokus_to_print',self.sudokusToPrintSpinButton.get_adjustment()),
                         ('sudokus_per_page',self.sudokusPerPageSpinButton.get_adjustment())
                         ]:
            self.gconf_wrap_adjustment(key,widg)
        self.dialog = self.glade.get_widget('dialog')
        self.dialog.set_default_response(gtk.RESPONSE_OK)
        self.dialog.connect('response',self.response_cb)

    def response_cb (self, dialog, response):
        if response not in (gtk.RESPONSE_ACCEPT, gtk.RESPONSE_OK):
            self.dialog.hide()
            return
        # Otherwise, we're printing!
        levels = []
        for cat in DR.categories:
            if getattr(self,
                       cat.replace(' ','_')+'CheckButton'
                       ).get_active():
                levels.append(cat)
        if not levels:
            levels = DR.categories.keys()
        nsudokus = self.sudokusToPrintSpinButton.get_adjustment().get_value()
        sudokus = self.sudoku_maker.get_puzzles(
            nsudokus,
            levels,
            new=not self.includeOldGamesToggle.get_active()
            )
        # Convert floating point difficulty into a label string
        sudokus.sort(cmp=lambda a,b: cmp(a[1],b[1]))
        sudokus = [(sudoku.sudoku_grid_from_string(puzzle),
                    "%s (%.2f)"%(sudoku.get_difficulty_category_name(d),d)
                    ) for puzzle,d in sudokus]
        from printing import SudokuPrinter
        sp = SudokuPrinter(sudokus,
                           sudokus_per_page=self.sudokusPerPageSpinButton.get_adjustment().get_value(),
                           dialog_parent=self.dialog)
        self.sudokus_printed = sudokus
        sp.run()
        sp.dialog.connect('response',
                          self.print_dialog_response_cb)

    def print_dialog_response_cb (self, dialog, response):
        if response == gnomeprint.ui.DIALOG_RESPONSE_CANCEL:
            #self.dialog.hide()
            pass
        elif response == gnomeprint.ui.DIALOG_RESPONSE_PREVIEW:
            pass
        elif response==gnomeprint.ui.DIALOG_RESPONSE_PRINT:
            if self.markAsPlayedToggle.get_active():
                for sud,lab in self.sudokus_printed:
                    jar = {}
                    jar['game']=sud.to_string()
                    jar['printed']=True
                    jar['printed_at']=time.time()
                    tracker = saver.SudokuTracker()
                    tracker.finish_jar(jar)
            self.dialog.hide()

    def run_dialog (self):
        #self.setup_dialog()
        self.dialog.show()

# if __name__ == '__main__':
#     try:
#         IMAGE_DIR='/usr/share/gnome-sudoku/'
#         import defaults
#         from gnome_sudoku import sudoku_maker
#         st = sudoku_maker.SudokuTracker(sudoku_maker.SudokuMaker(pickle_to='/tmp/foo'))
#         hs=HighScores(st)
#         hs.highlight_newest=True
#         hs.run_dialog()
#         st.save()
#     except:
#         import sys
#         print 'path was ',sys.path
#         raise
#
#if __name__ == '__main__':
#    gs = NewOrSavedGameSelector()
#    print 'DIALOG RETURNS:',gs.run_dialog()
#
#    import saver; sudoku_maker = sudoku_maker.SudokuMaker()
#    gs = GamePrinter(sudoku_maker,gconf_wrapper.GConf('gnome-sudoku'))
#    gs.run_dialog()
#    gtk.main()
