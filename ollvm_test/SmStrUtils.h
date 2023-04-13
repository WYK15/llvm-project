//
//  Copyright © 2017年 shumei. All rights reserved.
//  Pingshun Wei<weipingshun@ishumei.com>
//

#import <Foundation/Foundation.h>

@interface SmStrUtils : NSObject

+(BOOL) empty:(NSString*) str;

+(BOOL) notEmpty:(NSString*) str;

+(NSString*) trim:(NSString*) str;

+(BOOL) blank:(NSString*) str;

+(BOOL) notBlank:(NSString*) str;

+(NSString*) safe:(NSString*) str;

+(BOOL) equal:(NSString*) left right:(NSString*) right;

+(BOOL) notEqual:(NSString*) left right:(NSString*) right;

+(NSString *) randomString:(NSInteger) length;

+(NSString*) parseStr:(NSString*) str;

+ (NSString *) enc:(NSString *)str;
@end
