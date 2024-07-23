DIR_OF_THIS_SCRIPT = p.abspath( p.dirname( __file__ ) )

def Settings( **kwargs ):
  if language == 'cfamily':
    cmake_commands = os.path.join( DIR_OF_THIS_SCRIPT, 'build', 'compile_commands.json')
    if os.path.exists( cmake_commands ):
      return {
        'ls': {
          'compilationDatabasePath': os.path.dirname( cmake_commands )
        }
      }
  return None
