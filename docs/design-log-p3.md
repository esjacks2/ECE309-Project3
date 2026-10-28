# Project 3 Design Log
**Author:** <Erik Jackson> · **P2 code used:** <instructor reference>
**Tools used:** <ChatGPT - help with compiling errors and small debugs>
I used the Project 2 reference since my project 2 had issues compiling. I used Chat GPT to help find the issues with the compiling and small syntax debugs. It helped find that the compiling errors i was getting werre due to my own machines compiler version rather than actual code errors.


## 1. Hash table
For the ToolRegistry, I used a hash table with separate chaining. Each bucket contains a vector of pointers to a tool object. I chose separate chaining simply because it was the one im most familiar with and seemed the easiest to make over linear. If two tools end up in the same bucket then they can both be stored there without needing to find the next empty spot. I used the FNV-1a hash function because it uses every character in the tool name to find a hash value. I found this helps distribute tools across buckets and helps reduce collisions compared to a simple hash function.

The table starts with 16 buckets and doubles in size whenever adding another tool would cause the load factor to exceed 0.75. When it would resize, it would recalculate the bucket index for every stored tool and moves its unique pointer into the new table. Adding a tool has an average
time of O(1), even tho resizing takes O(n) time, it doesnt happen after every insertion. Most insertions only need to find the bucket and add the tool.

i tested the table using 10 thousand tools named tool_0 through tool_9999. The results were 
-Bucket count: 16,384
-Load Factor: 0.6104
-Max probe: 6
-Max xhain length using the weak hash: 9000
FNV-1a spread the tools across many buckets, while the weak hash caused most tools to end up in only  few buckets, which made searching much slower because it would have to check a large number of entries.
## 2. Pointer stability
I used std::unique_ptr because the registry needs to own the tools and automatically delete them. When the table resized, the unique pointers are moved to the new buckets, but the actual Tool objects stay at the same memory addresses. This means that a pointer returned by find() remains valid even after more tools are added and the hash table resizes.

## 3. Calculator trace
For the calculator, I used two stacks: one for numbers and one for operators. The program reads the tokens from left to right and follows operator precedence. For example, with 2 * (3 + 4) - 5, the program first pushes 2 onto the values stack and * onto the operator stack. It then pushes the opening parenthesis, followed by 3, +, and 4. When the closing parenthesis is reached, the calculator evaluates 3 + 4, giving 7. The opening parenthesis is then removed. When the subtraction operator is reached, multiplication is performed first, giving 2 * 7 = 14. Finally, the calculator subtracts 5, leaving 9 as the answer. The program also checks for division by zero and returns an error instead of trying to find an invalid result.

## 4. Errors as values
I made it so it would return errors as values instead of throwing exceptions for problems. For example, if a requested file did not exist or the valculator does something like divide by zero, then the tool returns an error message. This helps because the AI can jsut read the error and decide what to do rather than just stop the program, which exceptions are better for unexpected errors.

## 5. What I would design differently
If i had to do it again, I would probably spend more time comparing different hash functions and collisions. I used separate chaining because it was simple and straight forward but i would like to compare it to linear. i would have also liked to improve my testing which wouldve saved me more time, as well as ensure proper machine setup before spending hours trying to fix a compiler error.