__author__ = 'Robert Ancell <bob27@users.sourceforge.net>'
__license__ = 'GNU General Public License Version 2'
__copyright__ = 'Copyright 2005-2006  Robert Ancell'

import config
import player
import scene
import scene.cairo
import scene.opengl
import scene.human
import ui
import game
import chess.board

class MovePlayer(game.ChessPlayer):
    """This class provides a pseudo-player to watch for piece movements"""
    
    def __init__(self, view):
        """Constructor for a move player.
        
        'view' is the view to update
        """
        self.view = view
        game.ChessPlayer.__init__(self, '@view')

    # Extended methods

    def onPieceMoved(self, piece, start, end, delete):
        """Called by chess.board.ChessPlayer"""
        if self.view.moveNumber != -1:
            return
        animate = self.view.game.isStarted()
        p = self.view.scene.movePiece(piece, end, delete, animate)

        # If a player move notify when animation completes
        if animate and self.view.moveNumber == -1 and start is not None and start != end:
            self.view.scene.waitingPiece = p

    def onPlayerMoved(self, p, move):
        self.view._redrawHighlight()
            
    def onPlayerStartTurn(self, player):
        self.view._redrawHighlight()

class CairoPiece(scene.ChessPieceFeedback):
    """
    """
    
    model = None
    
    location = ''
    
    def __init__(self, scene, piece):
        self.scene = scene
        self.piece = piece

    def onDeleted(self):
        """Called by scene.ChessPieceFeedback"""
        self.scene.pieces.pop(self.piece)

    def onMoved(self):
        """Called by scene.ChessPieceFeedback"""
        self.scene.view._pieceMoved(self)

class SceneCairo(scene.SceneFeedback, scene.human.SceneHumanInput):
    """
    """
    controller = None
    
    # The game this scene is rendering
    game = None
    
    # TODO
    pieces       = None
    
    # FIXME: Abort when scenes changed
    waitingPiece = None

    def __init__(self, view):
        """
        """
        self.view = view
        self.controller = scene.cairo.Scene(self)
        self.game = view.game
        self.pieces = {}
        scene.human.SceneHumanInput.__init__(self)
        
    def getPieces(self):
        return self.pieces.values()

    def movePiece(self, piece, location, delete, animate):
        """
        """
        # Get the model for this piece creating one if it doesn't exist
        try:
            p = self.pieces[piece]
        except KeyError:
            # Make the new model
            pieceName = {chess.board.PAWN: 'pawn', chess.board.ROOK: 'rook', chess.board.KNIGHT: 'knight',
                         chess.board.BISHOP: 'bishop', chess.board.QUEEN: 'queen', chess.board.KING: 'king'}[piece.getType()]
            chessSet = {chess.board.WHITE: 'white', chess.board.BLACK: 'black'}[piece.getColour()]
            p = CairoPiece(self, piece)
            p.model = self.controller.addChessPiece(chessSet, pieceName, location, p)
            self.pieces[piece] = p

        # Move the model
        p.location = location
        p.model.move(location, delete, animate)
        
        return p

    # Extended methods

    def onRedraw(self):
        """Called by scene.cairo.Scene"""
        if self.game.view is not None and self.game.view.controller is not None:
            self.game.view.controller.render()

    def startAnimation(self):
        """Called by scene.cairo.Scene"""
        self.game.application.ui.controller.startAnimation()
        
    def getSquare(self, x, y):
        """Called by scene.human.SceneHumanInput"""
        return self.controller.getSquare(x, y)

    def playerIsHuman(self):
        """Called by scene.human.SceneHumanInput"""
        return self.game.currentPlayerIsHuman()

    def squareIsFriendly(self, coord):
        """Called by scene.human.SceneHumanInput"""
        return self.playerIsHuman() and self.game.squareIsFriendly(coord)
    
    def canMove(self, start, end):
        """Called by scene.human.SceneHumanInput"""
        return self.playerIsHuman() and self.game.getCurrentPlayer().canMove(start, end) # FIXME: Promotion type

    def selectSquare(self, coord):
        """Called by scene.human.SceneHumanInput"""
        self.view.setSelectedSquare(coord)

    def moveHuman(self, start, end):
        """Called by scene.human.SceneHumanInput"""
        self.game.moveHuman(start, end)
        
class OpenGLPiece(scene.ChessPieceFeedback):
    """
    """
    
    model = None
    
    location = ''
    
    def __init__(self, scene, piece):
        self.scene = scene
        self.piece = piece

    def onDeleted(self):
        """Called by scene.ChessPieceFeedback"""
        self.scene.pieces.pop(self.piece)

    def onMoved(self):
        """Called by scene.ChessPieceFeedback"""
        self.scene.view._pieceMoved(self)

class SceneOpenGL(scene.SceneFeedback, scene.human.SceneHumanInput):
    """
    """
    # The game this scene is rendering
    game          = None
    
    # TODO
    pieces        = None
    
    # FIXME: Abort when scenes changed
    waitingPiece = None

    def __init__(self, view):
        """Constructor for a glChess scene.
        

        """
        self.view = view
        self.game = view.game
        self.pieces = {}

        # Call parent constructors
        scene.human.SceneHumanInput.__init__(self)
        self.controller = scene.opengl.Scene(self)

    def getPieces(self):
        return self.pieces.values()
        
    def movePiece(self, piece, location, delete, animate):
        """
        """
        # Get the model for this piece creating one if it doesn't exist
        try:
            p = self.pieces[piece]
        except KeyError:
            # Make the new model
            pieceName = {chess.board.PAWN: 'pawn', chess.board.ROOK: 'rook', chess.board.KNIGHT: 'knight',
                         chess.board.BISHOP: 'bishop', chess.board.QUEEN: 'queen', chess.board.KING: 'king'}[piece.getType()]
            chessSet = {chess.board.WHITE: 'white', chess.board.BLACK: 'black'}[piece.getColour()]
            p = OpenGLPiece(self, piece)
            p.model = self.controller.addChessPiece(chessSet, pieceName, location, p)
            self.pieces[piece] = p
            
        # Move the model
        p.location = location
        p.model.move(location, delete)

        return p

    # Extended methods

    def onRedraw(self):
        """Called by scene.opengl.Scene"""
        if self.game.view is not None and self.game.view.controller is not None:
            self.game.view.controller.render()

    def startAnimation(self):
        """Called by scene.opengl.Scene"""
        self.game.application.ui.controller.startAnimation()
        
    def getSquare(self, x, y):
        """Called by scene.human.SceneHumanInput"""
        return self.controller.getSquare(x, y)

    def playerIsHuman(self):
        """Called by scene.human.SceneHumanInput"""
        return self.game.currentPlayerIsHuman()

    def squareIsFriendly(self, coord):
        """Called by scene.human.SceneHumanInput"""
        return self.playerIsHuman() and self.game.squareIsFriendly(coord)
    
    def selectSquare(self, coord):
        """Called by scene.human.SceneHumanInput"""
        self.view.setSelectedSquare(coord)
    
    def moveHuman(self, start, end):
        """Called by scene.human.SceneHumanInput"""
        self.game.moveHuman(start, end)

class Splashscreen(ui.ViewFeedback):
    """
    """    
    application = None
    scene       = None
    
    def __init__(self, application):
        """Constructor.
        
        'application' is ???
        """
        self.application = application
        self.cairoScene = scene.cairo.Scene(self)
        self.scene = scene.opengl.Scene(self)

    def updateRotation(self, animate = True):
        boardView = config.get('board_view')
        if boardView == 'black':
            rotation = 180.0
        else:
            rotation = 0.0
        self.cairoScene.controller.setBoardRotation(rotation, animate)
        self.scene.controller.setBoardRotation(rotation, animate)

    def onRedraw(self):
        """Called by scene.SceneFeedback"""
        if self.controller is not None:
            self.controller.render()
        
    def showBoardNumbering(self, showNumbering):
        """Called by ui.ViewFeedback"""
        self.scene.showBoardNumbering(showNumbering)
        self.cairoScene.showBoardNumbering(showNumbering)

    def renderGL(self):
        """Called by ui.ViewFeedback"""
        self.scene.render()
        
    def renderCairoStatic(self, context):
        """Called by ui.ViewFeedback"""
        return self.cairoScene.renderStatic(context)
        
    def renderCairoDynamic(self, context):
        """Called by ui.ViewFeedback"""
        self.cairoScene.renderDynamic(context)
    
    def reshape(self, width, height):
        """Called by ui.ViewFeedback"""
        self.scene.reshape(width, height)
        self.cairoScene.reshape(width, height)

    def selectSquare(self, coord):
        pass

    def resign(self):
        """Called by ui.ViewFeedback"""
        pass
    
    def claimDraw(self):
        """Called by ui.ViewFeedback"""
        pass
    
    def setMoveNumber(self, moveNumber):
        """Called by ui.ViewFeedback"""
        pass

class View(ui.ViewFeedback):
    """
    """
    # The controller object for this view
    controller  = None
    
    def __init__(self, game):
        """Constructor.
        
        'game' is the game this view is rendering
        """
        self.game = game
        
        # The move we are watching
        self.moveNumber = -1
        
        # The selected square
        self.selectedCoord = None
        self.showHints = False
        self.highlightParams = (None, None, None, None)
        self.changedHighlight = True
        
        # Use a Cairo scene by default - it will be replaced by an OpenGL one if that is the requested view
        # I wanted to remove this but then scene is None and there are a number of issues...
        # This should be cleaned up
        self.scene = SceneCairo(self)
        config.watch('board_view', self.__onBoardViewChanged)

        # Look for game events to update the scene
        movePlayer = MovePlayer(self)
        game.addSpectator(movePlayer)

    def setSelectedSquare(self, coord):
        if self.selectedCoord == coord:
            return
        self.selectedCoord = coord
        self._redrawHighlight()

    def _redrawHighlight(self):
        self.changedHighlight = True
        if self.controller is not None:
            self.controller.render()

    def __onBoardViewChanged(self, key, value):
        self.updateRotation()

    def _updateHighlight(self, coord):
        if self.moveNumber == -1:
            player = self.game.getCurrentPlayer()
            if player is self.game.getWhite():
                colour = chess.board.WHITE
            else:
                colour = chess.board.BLACK
        else:
            if self.moveNumber % 2 == 0:
                colour = chess.board.WHITE
            else:
                colour = chess.board.BLACK

        # Don't update if nothing has changed
        params = (colour, self.moveNumber, self.selectedCoord, self.showHints)
        if self.highlightParams == params:
            return
        self.highlightParams = params

        highlights = {}
        if self.showHints:
            for file in '12345678':
                for rank in 'abcdefgh':
                    c = rank + file
                    highlight = None
                    if self.game.board.squareUnderAttack(colour, c, moveNumber = self.moveNumber):
                        highlight = scene.HIGHLIGHT_THREATENED
                    elif coord is not None:
                        move = self.game.board.testMove(colour, coord, c, moveNumber = self.moveNumber)
                        if move is not None:
                            if self.game.board.getPiece(c, self.moveNumber):
                                highlight = scene.HIGHLIGHT_CAN_TAKE
                            else:
                                highlight = scene.HIGHLIGHT_CAN_MOVE

                    if highlight is not None:
                        highlights[c] = highlight
        if coord is not None:
            highlights[coord] = scene.HIGHLIGHT_SELECTED

        self.scene.controller.setBoardHighlight(highlights)

    def updateRotation(self, animate = True):
        """
        """
        # Get the angle to face
        p = self.game.getCurrentPlayer()
        if p is self.game.getWhite():
            rotation = 0.0
        elif p is self.game.getBlack():
            rotation = 180.0
        
        # Decide if we should face this angle
        boardView = config.get('board_view')
        if boardView == 'white':
            rotation = 0.0
        elif boardView == 'black':
            rotation = 180.0
        elif boardView == 'human':
            if not isinstance(p, player.HumanPlayer):
                return

        self.scene.controller.setBoardRotation(rotation, animate)

    def _pieceMoved(self, piece):
        """
        """
        self._redrawHighlight()
        
        # If waiting for this piece then end players turn
        if piece is self.scene.waitingPiece:
            self.scene.waitingPiece = None
            self.game.getCurrentPlayer().endMove()

    def showMoveHints(self, showHints):
        """Called by ui.ViewFeedback"""
        self.showHints = showHints
        self._redrawHighlight()

    def showBoardNumbering(self, showNumbering):
        """Called by ui.ViewFeedback"""
        self.scene.controller.showBoardNumbering(showNumbering)

    def updateScene(self, sceneClass):
        """
        """
        if self.changedHighlight:
            self._updateHighlight(self.selectedCoord)
        self.changedHighlight = False
        
        if isinstance(self.scene, sceneClass):
            return
        self._pieceMoved(None)
        self.scene = sceneClass(self)
        self.reshape(self.width, self.height)
        self.setMoveNumber(self.moveNumber)
        self.updateRotation(animate = False)

    def renderGL(self):
        """Called by ui.ViewFeedback"""
        self.updateScene(SceneOpenGL)
        self.scene.controller.render()

    def renderCairoStatic(self, context):
        """Called by ui.ViewFeedback"""
        self.updateScene(SceneCairo)
        return self.scene.controller.renderStatic(context)
        
    def renderCairoDynamic(self, context):
        """Called by ui.ViewFeedback"""
        self.updateScene(SceneCairo)
        self.scene.controller.renderDynamic(context)

    def reshape(self, width, height):
        """Called by ui.ViewFeedback"""
        self.width = width
        self.height = height
        self.scene.controller.reshape(width, height)
    
    def select(self, x, y):
        """Called by ui.ViewFeedback"""
        self.scene.select(x, y)
    
    def deselect(self, x, y):
        """Called by ui.ViewFeedback"""
        self.scene.deselect(x, y)
    
    def setMoveNumber(self, moveNumber):
        """Called by ui.ViewFeedback"""
        self.moveNumber = moveNumber
        
        # Lock the scene if not tracking the game
        self.scene.enableHumanInput(moveNumber == -1)
        
        # Get the state of this scene
        piecesByLocation = self.game.getAlivePieces(moveNumber)
        
        # Remove any models not present
        requiredPieces = piecesByLocation.values()
        for piece in self.scene.getPieces():
            try:
                requiredPieces.index(piece.piece)
            except ValueError:
                piece.model.move(piece.location, True)
        
        # Move the models in the scene
        for (location, piece) in piecesByLocation.iteritems():
            self.scene.movePiece(piece, location, False, True)

        # Can't wait for animation if not looking at the latest move
        if moveNumber != -1:
            self._pieceMoved(piece)

        self._redrawHighlight()

    def save(self, fileName = None):
        """Called by ui.ViewFeedback"""
        if fileName is None:
            fileName = self.game.fileName
            assert(fileName is not None)

        try:
            f = file(fileName, 'w')
        except IOError, e:
            return e.args[1]
        
        self.game.application.logger.addLine('Saving game %s to %s' % (repr(self.game.name), fileName))

        pgnGame = chess.pgn.PGNGame()
        self.game.toPGN(pgnGame)
            
        lines = pgnGame.getLines()
        for line in lines:
            f.write(line + '\n')
        f.write('\n')
        f.close()
        
        self.game.fileName = fileName
        
    def getFileName(self):
        """Called by ui.ViewFeedback"""
        return self.game.fileName
        
    def resign(self):
        """Called by ui.ViewFeedback"""
        p = self.game.getHumanPlayer()
        if p is not None:
            p.resign()
            
    def claimDraw(self):
        """Called by ui.ViewFeedback"""
        # TODO: Have the UI ask if the player wants to make a move first or claim now (or abort)
        p = self.game.getHumanPlayer()
        if p is not None:
            p.claimDraw()
