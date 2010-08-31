Sequence = @mark ( Statement )+ @collect

Lambda = OpenDelay @mark Tag_label? _ (theParameters In_kw)? Sequence? @collect CloseDelay

Access_list = @mark Access+ @collect
Access = @mark Access_label @collect

Signature = @mark
           ( Send_label (( Equal_kw Parameter ) | Parameter | Parameter_asList)?
           | Operator_label ( Parameter | Parameter_asList)
           | Parameter_asKeyword
           ) @collect

theParameters = ( Parameter_asList | Parameter )
Parameter_asList = Parameter_list
Parameter_list = Open @mark Parameter ( Comma_kw Parameter )+ ( Parameter_tail )? @collect Close
               | Open @mark Parameter   Parameter_tail @collect Close
Parameter_tail = @mark Rest_kw Parameter @collect

Parameter_asKeyword = @mark ( KeywordParameter )+ @collect
KeywordParameter    = @mark Tag_label Name_label ( TypeConstraint )? @collect

Parameter = @mark Name_label ( TypeConstraint )? @collect

Parameter_asAssignment = AssignForward_kw ( Parameter_asList | Parameter )

Variable_asList     = Variable_asList_fix | Variable_asList_var
Variable_asList_fix = @mark Open Variable ( Comma_kw Variable )* Close @collect
Variable_asList_var = @mark Open Variable ( Comma_kw Variable )* Rest_kw Variable Close @collect
                     | @mark Open Rest_kw Variable Close @collect
Variable  = @mark ( Name_label | Member_label | Skip_kw ) ( TypeConstraint )? @collect


Argument_asSelect = Open ( Type_name )+ Close

LetAssign = @mark theParameters Assign_kw Expression ( Lambda )? ( LineEnd | Rest_kw ) @collect _

Block           = @mark Sequence BlockClause* OtherwiseClause? FinallyClause? @collect
BlockClause     = TrapClause | CatchClause
TrapClause      = Trap_kw @mark Tag_label+ ( AssignForward_kw theParameters )? Sequence @collect
CatchClause     = Catch_kw @mark Expression (Comma_kw Expression)* ( AssignForward_kw theParameters )? _ Sequence @collect
OtherwiseClause = Otherwise_kw @mark Sequence @collect
FinallyClause   = Finally_kw @mark Sequence @collect

ContextBlock   = @mark ( Extends_kw Expression )? InContext* ( Block )? @collect
MethodBlock    = @mark InContext* Block @collect
PrototypeBlock = @mark InContext* Start_kw Parameter_asList ( _ Extends_kw Expression )? _ DefineMethod* Block @collect

InContext = DefineName | DeclareMember | DefineMethod

WhenClause = When_kw @mark Expression (Comma_kw Expression)* ( AssignForward_kw Name_label )? _ Block @collect
ElseClause = Else_kw @mark ( AssignForward_kw Name_label )? _ Block @collect

ForClause       = For_kw @mark theParameters In_kw Block @collect
EmptyClause     = Empty_kw @mark ( Open Name_label Comma_kw Name_label Close )? In_kw Block @collect
AmbiguousClause = Ambiguous_kw @mark ( Open Name_label Comma_kw Name_label Comma_kw Name_label Close )? In_kw Block @collect


Context      = Context_kw @mark ContextBlock @collect End_kw
Prototype    = Prototype_kw @mark PrototypeBlock @collect End_kw
Continuation = Continuation_kw @mark AssignForward_kw Name_label _ Block @collect End_kw
As           = As_kw @mark Tag_label? _ Block @collect End_kw
Fork         = Fork_kw @mark (Tag_label _ Block)+ @collect End_kw

Do = Do_kw @mark     Tag_label? _ (theParameters In_kw)? Block @collect End_kw
   | OpenDelay @mark Tag_label? _ (theParameters In_kw)? Sequence @collect CloseDelay

Keyword_send = @mark Keyword_arg (Tag_label Keyword_arg)+ @collect
Keyword_arg  = Operator_send | Simple_send | Value

Operator_send = @mark Operator_arg Operator_label Operator_arg @collect
Operator_arg  = Simple_send | Value

Simple_send = Do_send | Put_send | Cal_send | Get_send

Do_send   = @mark Get_send Do @collect
Put_send  = @mark Get_send Assign_kw Expression @collect
Cal_send  = @mark Get_send &Open Expression @collect
Get_send  = @mark Value ( Send_label )+ @collect

Value = @mark Name_label @collect
      | @mark Member_label @collect
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

List_value  = Open @mark Expression (Comma_kw Expression)* @collect Close
Array_value = OpenIndex @mark Expression (Comma_kw Expression)* @collect CloseIndex

Hash_value   = OpenIndex @mark Tagged_value (Comma_kw Tagged_value)* @collect CloseIndex
Tagged_value = @mark Tag_label Expression @collect

Constant  = @mark
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
          ) @collect       

KEYWORD = Ambiguous_kw
        | As_kw
        | AssignForward_kw
        | Assign_kw
        | Begin_kw
        | Case_kw
        | Catch_kw
        | Comma_kw
        | Context_kw
        | Continuation_kw
        | Continue_kw
        | Declare_kw
        | Define_kw
        | Directive_kw
        | Do_kw
        | Done_kw
        | Else_kw
        | Empty_kw
        | End_kw
        | Ensure_kw
        | Extends_kw
        | Equal_kw
        | Finally_kw
        | For_kw
        | Fork_kw
        | If_kw
        | In_kw
        | Is_kw
        | Join_kw
        | Let_kw
        | Loop_kw
        | Meet_kw
        | Next_kw
        | On_kw
        | Otherwise_kw
        | Prototype_kw
        | Reply_kw
        | Rest_kw
        | Return_kw
        | Select_kw
        | Signal_kw
        | Skip_kw
        | Start_kw
        | Stop_kw
        | Takes_kw
        | Throw_kw
        | Trap_kw
        | Type_kw
        | Typeof_kw
        | Unless_kw
        | When_kw
        | With_kw
        | VoidType_kw
        | BooleanType_kw
        | ImmutableType_kw
        | SymbolType_kw
        | TypeType_kw
        | ListType_kw
        | ArrayType_kw
        | HashType_kw
        | DelayType_kw
        | LambdaType_kw
        | SetType_kw
