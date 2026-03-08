---
paths:
  - "src/**"
---
# Documentation Conventions

## Doxygen

- All public functions and types must have Doxygen comments
- Header file block: `@file`, `@author`, `@brief`, `@version`, `@date`,
  `@copyright`
- Functions: `@param`, `@return`, `@note`, `@warning` as applicable
- Inline field docs: `/*!< ... */`

## Comments

- Big functions: short descriptive comments for main logic blocks
- Do not describe what is obvious from immediately following code
- Omit parameter docs when purpose is clear from name and type
- Update documentation when changing behavior or parameters
