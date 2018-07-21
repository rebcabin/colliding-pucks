	/* CIRCLES Brian Beckman, 1987 */

	/*******************************************************************/
	/*                                                                 */
	/*                     I n t r o d u c t i o n                     */
	/*                                                                 */
	/*******************************************************************/

	/*
        This package constitutes a "little language", in the
        sense that B. Kernighan has introduced that term, for
        manipulating circles, lines, and points on the plane.  It
        supports kinematic and dynamic operations with these
        objects, and also provides display and I/O functions.  It
        consists of a library of callable C functions and also a
        simple interpreter which can also be called from C.

        This file contains documentation and actual C code
        declaring data types and functions. The first section
        provides a summary and quick reference to the package.
        This is probably the most useful section for getting used
        to the package and learning it quickly.  Subsequent
        sections cover each ADT in detail, going over Times,
        Points, Vectors, LineSegments, Circles, and Collisions.

        The "universe of discourse" of this package contains
        moving circles and stationary points, vectors, and line
        segments. Dynamically, the circles behave like perfectly
        elastic hockey pucks of varying radius and mass.  Circles
        are considered to collide when their edges touch.
        Dynamically, points and lines behave like stationary
        obstacles with infinite mass, that is, circles rebound
        elastically from them without disturbing them.

        The package consists of a set of abstract data types
        (ADT's).  Each ADT consists of (1) a declaration of a
        data object type and its attributes (2) a set of
        functions implementing the allowed operations on
        instances of the ADT. We follow a certain convention for
        identifiers in this package. Data types ("typdefs" in C)
        are capitalized and the names of functions begin with a
        lower case letter.  Otherwise, upper and lower case
        characters are mixed in identifiers in the style of
        Smalltalk or Macintosh system software.

        There are certain elementary operations common to all
        ADT's in the package.  They include prompting, input, and
        output; and graphics operations, for ADT's with a
        graphical meaning. For the purposes of the following
        discussion, let the string "Type" stand for the names of
        real ADT's such as "Point", "Vector", etc.  Accordingly,
        an ADT called "Type" has the following elementary
        functions: "promptType", "readType", "scanType",
        "printType"; and "drawType" and "eraseType" if
        appropriate.  Let us describe these functions in more
        detail in the next few paragraphs.


        PromptType has the form, in C, "void promptType ()".
        That is, it takes no arguments and returns no value. Its
        side effect is to print a prompt string appropriate to
        type "Type" on the stderr channel of the calling process.
        This prompt string specifies the names and order of the
        elementary values expected by the input routine,
        readType.

        ReadType has the form, in C, "Type readType ()".  Thus,
        readType takes no arguments and returns an instance of
        type Type.  ReadType reads elementary attribute values
        from the stdin channel of the calling process and
        constructs an instance of type Type out of the elementary
        attributes.  The values must be presented in the order
        specified by the prompt routine promptType.

        ScanType has the form, in C, "Type scanType(string) char
        * string;". Thus, it takes a string argument and returns
        an instance of type Type.  It functions similarly to
        readType, reading elementary attribute values from the
        specified string rather than from the stdin channel of
        the calling process.  The order of elementary values in
        the string must be identical to the order specified by
        promptType, just as with readType.

        PrintType has the form, in C, "void printType (instance)
        Type instance;".  Thus, it takes an instance of type Type
        and returns nothing.  Its side effect is to print the
        formatted elementary values of its attributes on the
        stderr channel of the calling process.

        DrawType has the form, in C, "void drawType (instance)
        Type instance;".  Thus, it takes an instance of type Type
        and returns nothing.  Its side effect is to output a
        "pad" command on the stdout channel of the calling
        process.  Pad commmands are intended for the interactive
        graphics sketch pad program "pad".  Pad resides in
        /usr/local/bin.  To learn more about pad, run it and type
        "help" at its command prompt.  The pad commands output by
        drawType (or, more precisely, the process calling
        drawType) can be saved in a file or directed to pad
        through pseudoteletypes or other means.  The fact that
        they emerge from the stdout channel of the calling
        process gives the user all of UNIX's flexibility in
        manipulating them.  EraseType has a form similar to
        drawType, but issues appropriate pad commands to erase a
        previously drawn picture.


        Naturally, some ADT's will have special-purpose functions
        in addition to versions of those functions common to all
        ADT's.  The special functions implement an ADT's special
        behaviors such as moving around the surface or special
        mathematical operations.  After this discussion of common
	functions, it will be profitable to review the summary
	and quick reference guide section of this file.  
	Recognize the common functions for each ADT by their
	forms, discussed above, and distinguish them from the 
	special routines of each ADT as you read the summary.

        We do not go to the extreme, in general, of providing
        accessor functions (some ADT enthusiasts DO INSIST on
        them).  Instead, we assume that the user can access the
        attributes of an instance of an ADT using C's "." and "->" 
	operators.  Also, some ADT's have allowed operations
        identical to those for elementary data types.  For
        example, the "time" ADT's have "plus" and "minus"
        operations.  We assume the user of an ADT will feel free
        to use C's arithmetic and other operators to implement
        these common functions.  Our rather loose attitude toward
        the ADT discipline is consistent with the goal of this
        package to be helpful and easy to use without being (a)
        so large that it is time-consuming to implement and test
        (b) overly restrictive to the user.  */


	/*******************************************************************/
	/*                                                                 */
	/*      S U M M A R Y   A N D   Q U I C K   R E F E R E N C E      */
	/*                                                                 */
	/*******************************************************************/

	/*
	This summary is not actual C code, but rather represents 
	the information in a more human-readable format.  Detailed
	documentation follows this summary.

typedef ... AbsTime
typedef ... DeltaTime
typedef ... Point 
typedef ... Vector 
typedef ... LineSegment 
typedef ... Circle 
typedef ... Collision

***************  T I M E  **********************************************
void		promptAbsTime		( )
AbsTime		readAbsTime		( )
AbsTime		scanAbsTime		( char * string; )
void		printAbsTime		( AbsTime at; )

void		promptDeltaTime		( )
DeltaTime	readDeltaTime		( )
DeltaTime	scanDeltaTime		( char * string; )
void		printDeltaTime		( DeltaTime dt; )

***************  P O I N T S  ******************************************
void		promptPoint		( )
Point		readPoint		( ) 
Point		scanPoint		( char * string; )
void		printPoint		( Point p; )
void		drawPoint		( Point p; )
void		erasePoint		( Point p; )
Point		constructPoint 		( double x, y; ) 

***************  V E C T O R S  ****************************************
void		promptVector		( )
Vector		readVector		( ) 
Vector		scanVector		( char * string; )
void		printVector		( Vector v; )
void		drawVector		( Point p; Vector v; )
void		eraseVector		( Point p; Vector v; )
Vector		constructVector 	( double x,y ; ) 
Vector  	constructVectFromPts	( Point p1, p2 ; ) 
Point		otherEndPoint 		( Point p; Vector v; ) 
double		magnitudeVector 	( Vector input; ) 
double  	magnitudeSquaredVector 	( Vector input; ) 


Vector		vectorTimesScalar	( Vector v; double scalar; )
Vector  	vectorReverse 		( Vector input; )
Vector		unitVector		( Vector input; )
Vector		clockwiseNormal		( Vector input; )
Vector		anticlockwiseNormal	( Vector input; )

Vector  	sumVectors 		( Vector v1, v2 ; ) 
Vector  	differenceVectors 	( Vector v1, v2 ; ) 
double  	dotVectors 		( Vector v1, v2 ; ) 
double  	crossVectors 		( Vector v1, v2 ; ) 

***************  L I N E   S E G M E N T S  **************************** 
void		promptLineSegment	( ) 
LineSegment	readLineSegment		( ) 
LineSegment	scanLineSegment		( char * string; )
void		printLineSegment	( LineSegment l; )
void		drawLineSegment		( LineSegment l; )
void		eraseLineSegment	( LineSegment l; )
void		drawLineSegmentAndN	( LineSegment l; )
void		eraseLineSegmentAndN	( LineSegment l; )
LineSegment  	constructLineSegment 	( Point e; Vector l;  ) 

***************  C I R C L E S  **************************************** 
void		promptCircle		( )
Circle		readCircle		( ) 
Circle		scanCircle		( char * string; )
void		printCircle		( Circle * c; )
void		drawCircle		( Circle * c; )
void		eraseCircle		( Circle * c; )
void		drawCircleAndVelocity	( Circle * c; )
void		eraseCircleAndVelocity	( Circle * c; )
Circle		constructCircle 	( Point p; Vector v; double r, m;
					  AbsTime t; )

Vector		momentumCircle		( Circle * c; )
double		energyCircle		( Circle * c; )

Point		whereCircCtr		( Circle * c; AbsTime t; )
Point		whereCircCtrDeltaT	( Circle * c; DeltaTime t; )

Circle		moveCircle		( Circle * c; AbsTime t; )
Circle		moveCircleDeltaT	( Circle * c; DeltaTime t; )

***************  C O L L I S I O N S  ********************************** 
Collision	circCtrWLineSegment	( Circle * c; LineSegment l; )
Collision	circEdgeWLineSegment	( Circle * c; LineSegment l; )
Collision	circEdgeWithPoint 	( Circle * c; Point p; )
Collision	circleWithCircle	( Circle * c1, * c2; )

Collision	circLineSegDepart_SE	( Circle * c; LineSegment l; dis_dir; )
Collision	circPointIntersect_SE   ( Circle * c; Point p; )
Collision	circLineSegmentIntersect_SE ( Circle * c; LineSegment l; )
Collision	circAfterLineSegColl_SE	( Circle * c; LineSegment l; )
Collision	circlesAfterCircColl_SE	( Circle * c1, * c2; )
Collision	circleAfterPointColl_SE ( Circle * c; Point p; )

***************  M I S C E L L A N E O U S  **************************** 
int		testCircles		() 			      */

	/*******************************************************************/
	/*                                                                 */
	/*                             T I M E S                           */
	/*                                                                 */
	/*******************************************************************/

	/*
        This package is meant to support simple simulations of
        dynamics of "hockey pucks" moving in space.  We need time
        variables in such simulations.  Time Warp supplies the
        VirtualTime and Vtime data types; internally to this
        simulation, we use AbsTime and DeltaTime data types.
        Values of type AbsTime are taken to measure time from the
        beginning of the simulation at time zero, and DeltaTime
        values measure time with respect to particular AbsTime
        values.         */

typedef double	AbsTime ;
typedef double	DeltaTime ;

void		promptAbsTime		( ) ;
AbsTime		readAbsTime		( ) ;
AbsTime		scanAbsTime		( /* char * string; */ ) ;
void		printAbsTime		( /* AbsTime at; */ ) ;

void		promptDeltaTime		( ) ;
DeltaTime	readDeltaTime		( ) ;
DeltaTime	scanDeltaTime		( /* char * string; */ ) ;
void		printDeltaTime		( /* DeltaTime dt; */ ) ;



	/*******************************************************************/
	/*                                                                 */
	/*                           P O I N T S                           */
	/*                                                                 */
	/*******************************************************************/

	/*
	Points on the plane are represented by their positions with
	respect to a fixed, global rectangular coordinate grid.  */

typedef struct
{
    double	x, y ;
}
    Point ;

	/*
	The constructor function for instances of type Point takes
	the coordinates of the point and returns a Point.  */

Point	constructPoint ( /*  double x, y;  */  ) ;

void		promptPoint		( ) ;
Point		readPoint		( )  ;
Point		scanPoint		( /* char * string; */ ) ;
void		printPoint		( /* Point p; */ ) ;
void		drawPoint		( /* Point p; */ ) ;
void		erasePoint		( /* Point p; */ ) ;


	/*******************************************************************/
	/*                                                                 */
	/*                         V E C T O R S                           */
	/*                                                                 */
	/*******************************************************************/

	/*
        Vectors are directed lines, that is, they have a
        DIRECTION and a LENGTH.  On paper, they are represented
        by arrows in pictures and by symbols with arrows over-
        marks in equations. Numerically, vectors are represented
        by their components.  A vector has two components, one
        for the x direction and one for the y direction.  Each
        component is the length of the projection of the vector
        on the corresponding coordinate axis.  By convention, a
        component is positive if the projection of the vector on
        the corresponding coordinate axis points in the same
        direction as the coordinate axis, and the component is
        negative if the projection points in the direction
        opposite to the coordinate axis.

        Mathematically, vectors do not reside at any particular
        place on the plane, rather they can be moved around to
        wherever they are needed.

        There may be confusion over the fact that the coordinates
        of Points and the components of Vectors are formally the
        same, that is, that both Points and Vectors are
        represented by pairs of numbers. The resolution of the
        confusion lies in the fact that associated with each
        point is a vector pointing from the origin of coordinates
        to the point, and the components of that vector are
        numerically equal to the coordinates of the point.  A
        vector, in C, is */

typedef struct
{
    double	x, y ;  	/* x component, y component */
}
    Vector ;

void		promptVector		( ) ;
Vector		readVector		( ) ;
Vector		scanVector		( /* char * string; */ ) ;
void		printVector		( /* Vector v; */ ) ;

	/*
	The constructor for Vectors accepts a pair of components 
	and returns a Vector.  */

Vector	constructVector (  /* double x,y ; */ ) ;


	/*
        There is another constructor that will give you the
        vector connecting any pair of Points, p1 and p2.  This
        vector will point from p1 to p2.  Once this vector is
        constructed, it becomes mathematically like any other
        vector, that is, it becomes detached from its endpoints.
        This routine sets the x component of its answer to p2.x -
        p1.x, and the y component of its answer to p2.y - p1.y.  */

Vector  constructVectFromPts (  /*  Point p1, p2 ;  */ ) ;

	/*
        The drawVector routine is somewhat special among draw
        routines.  It requires a Point as well as a vector since
        it needs to know where to draw the vector from (remember
        that vectors can be translated all over the plane,
        wherever they are needed).  */

void	drawVector ( /*  Point p; Vector v;  */ ) ;
void	eraseVector ( /* Point p; Vector v;  */ ) ;

	/*
        There is a routine that will return a Point, p2, given
        another point, p1, and a vector, v, which is to be
        thought of as pointing from p1 to p2.  Mathematically,
        one is to think of the vector's having been translated to
        the specified point, p1, and the function returns the
        Point at the other end of the vector.  */

Point	otherEndPoint (  /*  Point p1; Vector v;  */ ) ;

	/*
	One attribute of a vector is its magnitude, that is, its 
	length.  This attribute is not stored with a Vector, so
	we provide a function to compute it.  */

double	magnitudeVector ( /*  Vector input;  */ ) ;

	/*
        Often, we need the squared magnitude of a vector.  For
        effciency reasons, we provide a routine providing this
        value directly.  Without this routine, one would have to
        call magnitudeVector, which does a square root, and then
        square the result.  */

double	magnitudeSquaredVector ( /*  Vector input;  */ ) ;

	/* 
        One useful thing to do with a vector is multiply it by a
        scalar.  This multiplication has the effect of changing
        the length of the vector without affecting its direction.  */

Vector	vectorTimesScalar ( /*  Vector input; double scalar; */ ) ;


	/* 
	Another useful operation is reversing a vector.  */

Vector	vectorReverse ( /*  Vector input;  */ ) ;

	/*
        One would often like to have a unit vector given a
        vector.  The unit vector points in the same direction as
        the input vector but has unit magnitude or length.  These
        vectors are extremely useful for specifying directions
        without resorting to trigonometry.  */

Vector	unitVector ( /*  Vector input ;  */ ) ;

	/*
        Every vector has two normal vectors associated with it.
        'Normal' means perpendicular.  One normal can be derived
        from the vector by rotating it 90 degrees clockwise and
        the other can be derived by rotating the vector 90
        degress anticlockwise. */

Vector	clockwiseNormal ( /*  Vector input ;  */ ) ;
Vector  anticlockwiseNormal ( /*  Vector input ;  */ ) ;

	/*
	SumVectors returns the vector sum of its two input vectors.  */

Vector sumVectors 		(   /* Vector v1, v2 ; */   ) ;

	/*
	DifferenceVectors returns the vector difference of 
	its two input vectors in the sense of v1 - v2.  */

Vector differenceVectors 	(   /* Vector v1, v2 ; */   ) ;

	/*
	DotVectors returns the scalar dot product of its 
	two input vectors; that is, it returns 
	v1.x * v2.x  +  v1.y * v2.y */

double dotVectors 		(   /* Vector v1, v2 ; */   ) ;

	/*
	CrossVectors returns the scalar cross product of 
	its two input vectors; that is, it returns 
	v1.x * v2.y  -  v1.y * v2.x */

double crossVectors 		(   /* Vector v1, v2 ; */   ) ;


	/*******************************************************************/
	/*                                                                 */
	/*                   L I N E   S E G M E N T S                     */
	/*                                                                 */
	/*******************************************************************/

	/*
        A line segment is represented by one endpoint, e, a unit
        normal vector, n, and a vector, l, pointing from the
        endpoint e to the other endpoint (the other endpoint is
        implicit and unexpressed; it can be found by calling the
        Vector routine "otherEndPoint").  */

typedef struct
{
    Point	e ;		/*  an endpoint  */
    Vector	l ;		/*  vector to the other endpoint  */
    Vector	n ;		/*  a unit normal to the line  */
}
    LineSegment ;

void		promptLineSegment	( ) ;
LineSegment	readLineSegment		( ) ;
LineSegment	scanLineSegment		( /* char * string; */ ) ;
void		printLineSegment	( /* LineSegment l; */ ) ;
void		drawLineSegment		( /* LineSegment l; */ ) ;
void		eraseLineSegment	( /* LineSegment l; */ ) ;

	/*
        The unit perpendicular, n, strictly speaking, is
        redundant information.  The line is actually completely
        described by its endpoint e and its length vector l.  N
        is required in some calculations, however, and we have
        made the decision to store it along with e and l rather
        than to recompute it every time it is needed, because its
        computation entails a square root.  So, to create a line
        segment, specify a point and a vector and get back a
        variable of type LineSegment; the constructor function
        automatically fills in the value of n.  After
        constructing a LineSegment, DON'T CHANGE N.  Everything
        will break if you do. */

LineSegment  constructLineSegment (  /*  Point e; Vector l;  */ ) ;

	/*
	Line Segments have an extra 'draw' routine that also draws the
	unit normal vector along with the end Point and the length Vector.  */

void drawLineSegmentAndN ( /*  LineSegment l; */ ) ;
void eraseLineSegmentAndN ( /* LineSegment l; */ ) ;


	/*******************************************************************/
	/*                                                                 */
	/*                         C I R C L E S                           */
	/*                                                                 */
	/*******************************************************************/

	/*
        A circle is represented by its position, velocity,
        radius, mass, and a reference time.  The position is the
        point (Point) where the center of the circle lies at the
        reference time.  The units of the velocity are in points
        per unit time.  The reference time is measured with
        respect to time 0.0 in the global time coordinate system
        -- that is, it is an absolute time. */

typedef struct
{
    Point	p ;		/*  position  */
    Vector	v ;		/*  velocity  */
    double	r ;		/*  radius  */
    double	m ;		/*  mass  */
    AbsTime	t ;		/*  reference time  */
}
    Circle ;

void		promptCircle		( ) ;
Circle		readCircle		( )  ;
Circle		scanCircle		( /* char * string; */ ) ;
void		printCircle		( /* Circle * c; */ ) ;
void		drawCircle		( /* Circle * c; */ ) ;
void		eraseCircle		( /* Circle * c; */ ) ;
	/*
        Because a Circle is a big data structure (seven doubles
        -> 56 bytes on the SUN), we adopt the following calling
        convention for efficiency reasons.  To pass data from a
        variable of type Circle to a routine in this package,
        pass a POINTER to the variable.

        Most routines taking Circle Pointers do not change the
        data in the Circle pointed to. As we have said, the
        main rationale behind using pointers is to speed up the
        passing of data between routines.  We have segregated
        routines so that those described in this section, the
        CIRCLES, section, are guaranteed to be free of side-
        effects on input data.  Routines WITH side-effects are
        are isolated in the next section, on COLLISIONS.

        So, for example, if you have a circle, c, and you wish 
	to draw it, you say 
	
	    Circle c;

	    drawCircle (&c) ;
	
        and the data in c is unchanged.  Any routine in the
        CIRCLES section of this package that creates a new
        circle, say by constructing one or by moving one
        kinematically, the routine returns a new variable of 
	type Circle.


        There are some routines in the next section, the
        COLLISION section, that require a pointer to a Circle
        variable for a different reason: to return new Circle
        data TO the caller in a block of memory provided BY the
        The fact that they have side effects on input parameters
        will be clearly documented in the COLLISION section.

        We have the following constructor.  */

Circle	constructCircle (  /* Point p; Vector v; double r, m; AbsTime t; */ ) ;

	/*
	Circles have an extra draw routine that draws the 
	velocity vector along with the circle.  */

void	drawCircleAndVelocity ( /* Circle * c; */ ) ;
void	eraseCircleAndVelocity	( /* Circle * c; */ ) ;

	/*
	Circles have two computed attributes: momentum, a vector,
	and energy, a scalar.  */

Vector	momentumCircle ( /* Circle * c; */ ) ;

double	energyCircle ( /* Circle * c; */ ) ;

	/*
        Given a circle and a time, represented by an an AbsTime
        or a DeltaTime, where will the center of the circle be if
        it continues on its present course?  This question is
        answered by the following routines. */

Point	whereCircCtr ( /*  Circle * c;  AbsTime t;  */ ) ;

Point	whereCircCtrDeltaT ( /*  Circle * c;  DeltaTime t;  */ ) ;

	/*
        To translate a circle from it position at the reference
        time of the circle (stored as one of the circle's
        attributes) to its position at a given time, represented
        by an AbsTime or a DeltaTime and return the new circle,
        call the following routines.  Note that you can move
        circles backwards and forwards in time.  */

Circle	moveCircle ( /*  Circle * c; AbsTime t;  */ ) ;

Circle	moveCircleDeltaT  ( /*  Circle * c;  DeltaTime t;  */ ) ;


	/*******************************************************************/
	/*                                                                 */
	/*                      C O L L I S I O N S                        */
	/*                                                                 */
	/*******************************************************************/

	/*
        We need to ask questions about several types of
        collisions, like whether, when, and where they will
        occur.  We therefore define a Collision data type
        containing the answers to "whether" and "when".  There is
        no single answer to where since "where" means something
        different for each geometrical object type we have
        defined.  */
	
typedef struct
{
    AbsTime	at ;
    int		yes ;
}
    Collision ;

	/*
        The 'yes' attribute of a Collision value will have the
        value YES if the collision occurs, and in this case, the
        't' attribute of the Collision value will express the
        absolute time at which the collision occurs.  The values
        YES and NO have the property that they can be used like
        booleans in conditionals and tests, allowing code to be
        shorter and simpler.  For example, one might say

	    Collision  my_collision ;

	      ... find out whether the collision occurs ...

	    if ( my_collision.yes )
	    {
		... compute consequences of the collision ...
	    }
	*/

#ifdef YES	/*???PJH conflict with BBN style.h	*/
#undef YES
#undef NO
#endif

#define	YES   1
#define  NO   0


	/*
        A moving circle will either cross a stationary line
        segment or it will not.  If it does, the CENTER of the
        circle will touch the line segment at a particular
        instant of time, measured by an AbsTime value.  The
        answer to "whether?" and the answer to "when?" are neatly
        packaged in a Collision value returned by the following
        routine (the routine after this one predicts times at
	which the EDGE of a circle crosses a line).

        Note that the collision might occur at a time in the past
        of the reference time of the circle, that is, the 't'
        attribute of the Collision value might be less than the
        't' attribute of the Circle input value.  This condition
        will occur when the circle is heading away from the
        segment.  */

Collision	circCtrWLineSegment ( /* Circle * c; LineSegment l; */ ) ;


	/*
        When a moving circle is heading towards a line segment,
        the edge of the circle crosses the segment at a certain
        time, different from, and, in fact, earlier than, the
        time at which the center of the circle crosses the
        segment.

        There are several cases to discuss to explain what is
        meant by collisions between circles and line segments in
        this package and to explain how these routines behave.
        Consider the following sketch, in which a circle is
        depicted heading towards a line segment:

        CASE I:                                |                     
                                               |                     
                        ++++++++++             |                     
                     +++          +++          |                     
                    ++              ++         |                     
                    +                +         |                     
                   +        C -------------->  |                     
                    +                +         |                     
                    ++              ++         |                     
                     +++          +++          |                     
                        ++++++++++             |                     
                                               |                     
                                               |                     
                                                                               
        A collision will be predicted at the moment the leading
        edge of the circle crosses the line segment.  The
        predicted time will be greater than the reference time of
        the circle.

	Consider, now, the following sketch:

        CASE II:     |                                                        
                     |                                              
                     |            ++++++++++                                  
                     |         +++          +++                               
                     |        ++              ++                              
                     |        +                +                              
                     |       +        C -------------->                       
                     |        +                +                              
                     |        ++              ++                              
                     |         +++          +++                               
                     |            ++++++++++                                  
                     |                                              
                     |                                              

        In this case, no collision will be predicted by the
        routine because the circle is regarded as moving away
        from the line.  The next sketch shows a more subtle case.


	CASE III:              |                                    
                               |                                    
                        ++++++++++                                  
                     +++       |  +++                               
                    ++         |    ++                              
                    +          |     +                              
                   +        C -------------->                       
                    +          |     +                              
                    ++         |    ++                              
                     +++       |  +++                               
                        ++++++++++                                  
                               |                                    
                               |                                    
                                                                               
        In this case, a collision WILL BE PREDICTED, because the
        circle is regarded as heading towards the line since its
        center is heading towards the line.  However, the
        collision will be predicted for the time at which the
        leading edge of the circle crossed the line in the past,
        that is, the predicted time of the collision will be less
        than the current reference time of the circle.  The final
	sketch is the next one.

	CASE IV:               |                                         
                               |                                         
                             ++++++++++                                  
                          +++  |       +++                               
                         ++    |         ++                              
                         +     |          +                              
                        +      | C -------------->                       
                         +     |          +                              
                         ++    |         ++                              
                          +++  |       +++                               
                             ++++++++++                                  
                               |                                         
                               |                                         
	
        In this case, no collision will be predicted because the
        circle is regarded as heading away from the line since
        its center is heading away from the line.

        The final subtlety concerning the definition of circle
        and line segment collisions concerns collisions between
        the circle and the endpoints of the segment.  The routine
        currently under discussion only predicts collisions where
        the circle becomes tangent to the line segment.  Cases
        where the circle will strike the endpoints but will never
        lie tangent to the line segment are ignored by this
        routine, and it predicts no collision.  However, such
        cases can easily be taken into account by a user program
        which using the routine after this one, whose purpose is
        to predict collisions between circles and points.  */

Collision	circEdgeWLineSegment ( /* Circle * c; LineSegment l; */ ) ;


	/* 
	The edge of a circle can collide with a stationary point in 
	the plane.  The following routine predicts the time of such a 
	collision.  */

Collision	circEdgeWithPoint ( /*  Circle * c;  Point p;  */ ) ;

	/*
	Pairs of circles can collide, also, and the following routine 
	predicts the time of such a collision.  */
    
Collision	circleWithCircle ( /*  Circle * c1, * c2;  */ ) ;

	/*
        The previous three routines do not have any side effects
        on their input circles.  They serve only to predict
        whether and when collisions occur.

        When one also asks what dynamical effects a collision has
        on the objects involved, side effects on values come into
        play.  Dynamical effects change circle velocities and
        take into account the masses of objects.  The next
        routine treats a line segment as an infinitely massive
        fixed object, like a granite wall, and bounces a given
        circle off the wall.  This routine will modify the
        reference time, the position, and the velocity vector of
        the input circle.  The "_SE" in its name is a forceful
        reminder that this routine has side effects on its input
        value.  This routine incidentally answers the "whether"
        and "why" questions and returns an appropiate Collision
        value.  If the collision does not occur, the input data
        will not be disturbed.  In CASE III, discussed above,
        where a collision is predicted in the past of the circle,
        the circle is backed up to the time where its leading
        edge first touched the granite wall before the dynamical
        effects on the velocity of the circle are calculated.
        This effect of backing up the circle can lead to
        unexpected results if one is not careful in the use of
        this function.  */

Collision	circAfterLineSegColl_SE ( /*  Circle * c; LineSegment l; */ );


	/*
        The following routine is similar to the previous except
        that it produces the effects of a collision on a pair of
        circles.  It also has side effects on its input circles,
        returning to the caller the positions and velocities of
        the two circles after a rigid- body collision.  It sets
        the reference times of the two circles to the collision
        time, and the position and velocity attributes of the
        circles to their new values, after the collision.  Note
        that if the collision does not occur, the input circles
        are not disturbed. */

/*???PJH come back and document....	*/

Collision	circLineSegDepart_SE ( /*  Circle * c; LineSegment l; int dis_dir*/ );


Collision	circPointIntersect_SE ( /*  Circle * c; Point p; */ );

Collision	circLineSegmentIntersect_SE ( /*  Circle * c; LineSegment l; */ );


	/*
	This routine returns the collision data structure which  
	tells the caller whether and when the ball ( c ) will 
	intersect with line segment ( l ). What is meant by intersect
	here is the time at which the first edge point of the
	circle touches a point on the line segment. If an intersect
	does occur, this function updates the ball's state given
	in the argument ( c ). Intersect is different from the
	collision functions here in that the intersecting circle
	does not bounce of the line segment but passes through
	the line segment. 	*/


Collision	circlesAfterCircColl_SE ( /* Circle *c1, *c2; */ ) ;

	/*
        Finally, for dynamical effects, a point is considered to
        be an infinitely massive barrier off of which circles
        rebound elastically.  The following routine calculates
        the effects of such a collision and deposits the
        appropriate values in its input circle buffer, and
        incidentally computes and returns a Collision type.  */

Collision	circleAfterPointColl_SE ( /* Circle *c; Point p; */ ) ;


	/*******************************************************************/
	/*                                                                 */
	/*                         T E S T I N G                           */
	/*                                                                 */
	/*******************************************************************/

	/*
        All that remains is a testing routine that will allow the
        interac- tive calling of the routines in this package.
        This routine is called testCircles, and will return 1 so
        long as the user does not issue a quit command.  It is
        meant to be used in a loop; the following main program
        implements a Circles interpreter.

	    main ()
	    {
		while ( testCircles () ) ;
	    }

        The output of the testing routine (which comes out the
        stdout channel of the calling process) should be saved in
        a file and displayed with a pad process in another
        window.  */

int	testCircles () ;

