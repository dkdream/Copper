
TypeConstraint = @mark
               ( Is_kw     Type_name
               | In_kw     Argument_asType
               | Meet_kw   Argument_asType
               | Join_kw   Argument_asType
               | Takes_kw  Argument_asSelect
               ) @collect

## Parts of TypeConstraint  --------------------------------------------

Type_name = VoidType_kw
          | BooleanType_kw
          | ImmutableType_kw
          | SymbolType_kw
          | TypeType_kw
          | HashType_kw
          | ListType_kw
          | DelayType_kw
          | ArrayType_kw
          | LambdaType_kw
          | SetType_kw
          | Name_label

Argument_asType   = OpenSet ( Type_name )+ CloseSet

Argument_asSelect = Open ( Type_name )+ Open ##maybe

## Parts of --------------------------------------------

Typeof = @mark Typeof_kw Expression @collect

Type   = @mark Type_kw Type_arg @collect

## Parts of Type --------------------------------------------

Type_arg = Type_new | Type_graph

## Parts of Type_arg --------------------------------------------

Type_new = @mark Open Message_type (Comma_kw Message_type)* Close @collect

Type_graph = @mark
            ( In_kw   OpenIndex ( TypeGraph_arg )+ CloseIndex 
            | Meet_kw OpenIndex ( TypeGraph_arg )+ CloseIndex 
            | Join_kw OpenIndex ( TypeGraph_arg )+ CloseIndex
            ) @collect

## Parts of Type_new --------------------------------------------

Message_type = @mark ( Send_label ( Equal_kw )? ) @collect
             | @mark   Operator_label             @collect
             | @mark ( Tag_label )+               @collect

## Parts of Type_graph --------------------------------------------

TypeGraph_arg = Type_name | Type_new

