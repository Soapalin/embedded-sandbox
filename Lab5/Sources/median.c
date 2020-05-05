/*! @file
 *
 *  @brief Median filter.
 *
 *  This contains the functions for performing a median filter on byte-sized data.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-05-17
 */

/*!
 *  @addtogroup median_module median module documentation
 *  @{
*/
#include "types.h"
#include "median.h"



uint8_t Median_Filter3(const uint8_t n1, const uint8_t n2, const uint8_t n3)
{
  if ((n1 >= n2 && n1 <= n3) || (n1 >= n3 && n1 <= n2))
    return n1;
  if((n3 <= n2 && n3 >= n1) || (n3 <= n1 && n3 >= n2))
    return n3;
  else
    return n2;
}

/*!
 * @}
*/
