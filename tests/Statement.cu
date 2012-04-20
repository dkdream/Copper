
Statement = DefineName
          | DefineMethod
          | Directive
          | DeclareName
          | DeclareMember
          | Let
          | Case
          | Select
          | If
          | Unless
          | Begin
          | Loop
          | Continue
          | Stop
          | Assign
          | Reply
          | Signal
          | Throw
          | Next
          | Return
          | Expression _
          | NoOp

## Parts of Statement --------------------------------------------

NoOp            = Rest_kw _
DefineName      = @begin Define_kw Parameter Assign_kw Expression Lambda? _ @end
DefineMethod    = @begin Define_kw Access_list? Signature _ MethodBlock End_kw @end
Directive       = @begin Directive_kw End_kw @end
DeclareName     = @begin Declare_kw Parameter ( Assign_kw Expression )? _ @end
DeclareMember   = @begin Declare_kw Member_label ( TypeConstraint )? ( Assign_kw Expression )? _ @end
Let             = @begin Let_kw LetAssign+ In_kw Sequence End_kw @end
Case            = @begin Case_kw Expression _ (WhenClause _)* (ElseClause _)? End_kw @end
Select          = @begin Select_kw Expression _ ForClause* EmptyClause? AmbiguousClause? End_kw @end
If              = @begin If_kw Expression _ Block ( Else_kw Block )? End_kw @end
Unless          = @begin Unless_kw Expression _ Block ( Else_kw Block )? End_kw @end
Begin           = @begin Begin_kw Block End_kw @end
Loop            = @begin Loop_kw Tag_label? _ Block End_kw @end
Continue        = @begin Continue_kw Tag_label? _ @end
Stop            = @begin Stop_kw Tag_label? _ @end
Assign          = @begin ( Name_label | Member_label | Variable_asList ) Assign_kw Expression _ @end
Reply           = @begin Reply_kw Tag_label Expression _ @end
Signal          = @begin Signal_kw Tag_label Expression _ @end
Throw           = @begin Throw_kw Expression _ @end
Next            = @begin Next_kw @end
Return          = @begin Return_kw Expression _ @end
Expression      = @begin ( Typeof | Type | Keyword_send | Operator_send | Simple_send | Value ) @end

## Parts of NoOp  --------------------------------------------
## Parts of DefineName  --------------------------------------------

Parameter = ( Parameter_asList | Parameter_asName )
Lambda    = OpenDelay Tag_label? _ ( Parameter In_kw )? Sequence? CloseDelay

## Parts of DefineMethod --------------------------------------------

Access_list = Access+

Signature   =
              ( Send_label (( Equal_kw Parameter_asName ) | Parameter_asName | Parameter_asList)?
              | Operator_label ( Parameter_asName | Parameter_asList)
              | Parameter_asKeyword
              )
 
MethodBlock = InContext* Block

## Parts of Directive --------------------------------------------
## Parts of DeclareName --------------------------------------------
## Parts of DeclareMember --------------------------------------------
## Parts of Let --------------------------------------------

LetAssign  = @begin Parameter Assign_kw Expression ( Lambda )? ( LineEnd | Rest_kw ) _ @end
Sequence   = @begin ( Statement )+ @end

## Parts of Case --------------------------------------------

WhenClause = @begin When_kw Expression (Comma_kw Expression)* ( AssignForward_kw Name_label )? _ Block @end
ElseClause = @begin Else_kw ( AssignForward_kw Name_label )? _ Block @end

## Parts of Select --------------------------------------------

ForClause       = @begin For_kw Parameter In_kw Block @end
EmptyClause     = @begin Empty_kw ( Open Name_label Comma_kw Name_label Close )? In_kw Block @end
AmbiguousClause = @begin Ambiguous_kw ( Open Name_label Comma_kw Name_label Comma_kw Name_label Close )? In_kw Block @end

## Parts of If  --------------------------------------------

Block = @begin Sequence BlockClause* OtherwiseClause? FinallyClause? @end

## Parts of Unless  --------------------------------------------
## Parts of Begin  --------------------------------------------
## Parts of Loop  --------------------------------------------
## Parts of Continue  --------------------------------------------
## Parts of Stop  --------------------------------------------
## Parts of Assign  --------------------------------------------

Variable_asList = Variable_asList_fix | Variable_asList_var

## Parts of Reply  --------------------------------------------
## Parts of Signal  --------------------------------------------
## Parts of Throw  --------------------------------------------
## Parts of Next  --------------------------------------------
## Parts of Return  --------------------------------------------
## Parts of Expression  --------------------------------------------

Keyword_send  = Keyword_arg (Tag_label Keyword_arg)+
Operator_send = Operator_arg Operator_label Operator_arg
Simple_send   = Do_send | Put_send | Cal_send | Get_send

Value = Name_label
      | Member_label
      | Constant
      | List_value
      | Array_value
      | Hash_value
      | Do
      | Context
      | Prototype
      | Continuation
      | As
      | Fork
      | Type

## Parts of Parameter  --------------------------------------------

Parameter_asList = Open { Parameter_asName ; Comma_kw } ( Parameter_tail )? Close
                 | Open Parameter_asName Parameter_tail    Close

Parameter_asName = Name_label ( TypeConstraint )?

## Parts of Lambda  --------------------------------------------
## Parts of Access_list  --------------------------------------------

Access = Access_label

## Parts of Signature  --------------------------------------------

Parameter_asKeyword = ( KeywordParameter )+

## Parts of MethodBlock  --------------------------------------------

InContext = DefineName | DeclareMember | DefineMethod

## Parts of LetAssign  --------------------------------------------
## Parts of Sequence  --------------------------------------------
## Parts of WhenClause  --------------------------------------------
## Parts of ElseClause  --------------------------------------------
## Parts of ForClause  --------------------------------------------
## Parts of EmptyClause  --------------------------------------------
## Parts of AmbiguousClause  --------------------------------------------
## Parts of Block  --------------------------------------------

BlockClause     = TrapClause | CatchClause

OtherwiseClause = @begin Otherwise_kw Sequence @end

FinallyClause   = @begin Finally_kw Sequence @end

## Parts of Variable_asList --------------------------------------------

Variable_asList_fix =  Open Variable ( Comma_kw Variable )* Close

Variable_asList_var = Open Variable ( Comma_kw Variable )* Rest_kw Variable Close
                     | Open Rest_kw Variable Close

## Parts of Keyword_send  --------------------------------------------

Keyword_arg  = Operator_send | Simple_send | Value

## Parts of Operator_send  --------------------------------------------

Operator_arg  = Simple_send | Value

## Parts of Simple_send  --------------------------------------------

Do_send  = Get_send Do

Put_send = Get_send Assign_kw Expression

Cal_send = Get_send &Open Expression

Get_send = Value ( Send_label )+

## Parts of Value  --------------------------------------------

Constant  =
          ( void_kw
          | true_kw
          | false_kw
          | Number
          | Symbol_label
          | QSymbol_label
          | Text_variable
          | Text_constant
          | Regex_constant
          | Meta_constant
          )

List_value   = Open Expression ( Comma_kw Expression )* Close

Array_value  = OpenIndex Expression ( Comma_kw Expression )* CloseIndex

Hash_value   = OpenIndex Tagged_value ( Comma_kw Tagged_value )* CloseIndex

Do           = Do_kw     Tag_label? _ ( Parameter In_kw )? Block End_kw
             | OpenDelay Tag_label? _ ( Parameter In_kw )? Sequence CloseDelay

Context      = Context_kw ContextBlock End_kw

Prototype    = Prototype_kw PrototypeBlock End_kw

Continuation = Continuation_kw AssignForward_kw Name_label _ Block End_kw

As           = As_kw Tag_label? _ Block End_kw

Fork         = Fork_kw ( Tag_label _ Block )+ End_kw

## Parts of Parameter_asList  --------------------------------------------

Parameter_tail = Rest_kw Parameter_asName

## Parts of Parameter_asName  --------------------------------------------
## Parts of Access  --------------------------------------------
## Parts of Parameter_asKeyword  --------------------------------------------

KeywordParameter = Tag_label Name_label ( TypeConstraint )?

## Parts of InContext  --------------------------------------------
## Parts of BlockClause  --------------------------------------------

TrapClause  = Trap_kw Tag_label+ ( AssignForward_kw  Parameter )? Sequence

CatchClause = Catch_kw Expression ( Comma_kw Expression )* ( AssignForward_kw Parameter )? _ Sequence

## Parts of OtherwiseClause  --------------------------------------------
## Parts of FinallyClause  --------------------------------------------
## Parts of Variable_asList_fix  --------------------------------------------

Variable = ( Name_label | Member_label | Skip_kw ) ( TypeConstraint )?

## Parts of Variable_asList_var  --------------------------------------------
## Parts of Keyword_arg  --------------------------------------------
## Parts of Operator_arg  --------------------------------------------
## Parts of Do_send  --------------------------------------------
## Parts of Put_send  --------------------------------------------
## Parts of Cal_send  --------------------------------------------
## Parts of Get_send  --------------------------------------------
## Parts of Constant  --------------------------------------------
## Parts of List_value  --------------------------------------------
## Parts of Array_value  --------------------------------------------
## Parts of Hash_value  --------------------------------------------

Tagged_value = Tag_label Expression

## Parts of Do  --------------------------------------------
## Parts of Context  --------------------------------------------

ContextBlock = ( Extends_kw Expression )? InContext* ( Block )?

## Parts of Prototype  --------------------------------------------

PrototypeBlock = InContext* Start_kw Parameter_asList ( _ Extends_kw Expression )? _ DefineMethod* Block

## Parts of Continuation  --------------------------------------------
## Parts of As  --------------------------------------------
## Parts of Fork  --------------------------------------------
## Parts of Parameter_tail  --------------------------------------------
## Parts of KeywordParameter  --------------------------------------------
## Parts of TrapClause  --------------------------------------------
## Parts of CatchClause  --------------------------------------------
## Parts of Variable  --------------------------------------------
## Parts of Tagged_value  --------------------------------------------
## Parts of ContextBlock  --------------------------------------------
## Parts of PrototypeBlock  --------------------------------------------



